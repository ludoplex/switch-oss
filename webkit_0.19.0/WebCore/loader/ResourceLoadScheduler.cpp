/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
    Copyright (C) 2010 Google Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "ResourceLoadScheduler.h"

#include "Document.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "InspectorInstrumentation.h"
#include "URL.h"
#include "LoaderStrategy.h"
#include "Logging.h"
#include "NetscapePlugInStreamLoader.h"
#include "PlatformStrategies.h"
#include "ResourceLoader.h"
#include "ResourceRequest.h"
#include "SubresourceLoader.h"
#include <wtf/MainThread.h>
#include <wtf/TemporaryChange.h>
#include <wtf/text/CString.h>

#if PLATFORM(IOS)
#include <RuntimeApplicationChecksIOS.h>
#endif

#if USE(QUICK_LOOK)
#include "QuickLook.h"
#endif

namespace WebCore {

// Match the parallel connection count used by the networking layer.
#if !PLATFORM(WKC)
static unsigned maxRequestsInFlightPerHost;
#else
WKC_DEFINE_GLOBAL_UINT(maxRequestsInFlightPerHost, 0);
#endif
#if !PLATFORM(IOS)
static const unsigned maxRequestsInFlightForNonHTTPProtocols = 20;
#else
// Limiting this seems to regress performance in some local cases so let's just make it large.
static const unsigned maxRequestsInFlightForNonHTTPProtocols = 10000;
#endif

ResourceLoadScheduler::HostInformation* ResourceLoadScheduler::hostForURL(const URL& url, CreateHostPolicy createHostPolicy)
{
    if (!url.protocolIsInHTTPFamily())
        return m_nonHTTPProtocolHost;

    m_hosts.checkConsistency();
    String hostName = url.host();
    HostInformation* host = m_hosts.get(hostName);
    if (!host && createHostPolicy == CreateIfNotFound) {
        host = new HostInformation(hostName, maxRequestsInFlightPerHost);
        m_hosts.add(hostName, host);
    }
    return host;
}

ResourceLoadScheduler* resourceLoadScheduler()
{
    ASSERT(isMainThread());
#if !PLATFORM(WKC)
    static ResourceLoadScheduler* globalScheduler = 0;
#else
    WKC_DEFINE_STATIC_PTR(ResourceLoadScheduler*, globalScheduler, 0);
#endif
    
    if (!globalScheduler) {
#if !PLATFORM(WKC)
        static bool isCallingOutToStrategy = false;
#else
        WKC_DEFINE_STATIC_BOOL(isCallingOutToStrategy, false);
#endif
        
        // If we're re-entering resourceLoadScheduler() while calling out to the LoaderStrategy,
        // then the LoaderStrategy is trying to use the default resourceLoadScheduler.
        // So we'll create it here and start using it.
        if (isCallingOutToStrategy) {
            globalScheduler = new ResourceLoadScheduler;
            return globalScheduler;
        }
        
        TemporaryChange<bool> recursionGuard(isCallingOutToStrategy, true);
        globalScheduler = platformStrategies()->loaderStrategy()->resourceLoadScheduler();
    }

    return globalScheduler;
}

ResourceLoadScheduler::ResourceLoadScheduler()
    : m_nonHTTPProtocolHost(new HostInformation(String(), maxRequestsInFlightForNonHTTPProtocols))
    , m_requestTimer(*this, &ResourceLoadScheduler::requestTimerFired)
    , m_suspendPendingRequestsCount(0)
    , m_isSerialLoadingEnabled(false)
{
    maxRequestsInFlightPerHost = initializeMaximumHTTPConnectionCountPerHost();
}

ResourceLoadScheduler::~ResourceLoadScheduler()
{
}

PassRefPtr<SubresourceLoader> ResourceLoadScheduler::scheduleSubresourceLoad(Frame* frame, CachedResource* resource, const ResourceRequest& request, const ResourceLoaderOptions& options)
{
    RefPtr<SubresourceLoader> loader = SubresourceLoader::create(frame, resource, request, options);
    if (loader)
        scheduleLoad(loader.get());
#if PLATFORM(IOS)
    // Since we defer loader initialization until scheduling on iOS, the frame
    // load delegate that would be called in SubresourceLoader::create() on
    // other ports might be called in scheduleLoad() instead. Our contract to
    // callers of this method is that a null loader is returned if the load was
    // cancelled by a frame load delegate.
    if (!loader || loader->reachedTerminalState())
        return nullptr;
#endif
    return loader.release();
}

PassRefPtr<NetscapePlugInStreamLoader> ResourceLoadScheduler::schedulePluginStreamLoad(Frame* frame, NetscapePlugInStreamLoaderClient* client, const ResourceRequest& request)
{
    PassRefPtr<NetscapePlugInStreamLoader> loader = NetscapePlugInStreamLoader::create(frame, client, request);
    if (loader)
        scheduleLoad(loader.get());
    return loader;
}

void ResourceLoadScheduler::scheduleLoad(ResourceLoader* resourceLoader)
{
    ASSERT(resourceLoader);

    LOG(ResourceLoading, "ResourceLoadScheduler::load resource %p '%s'", resourceLoader, resourceLoader->url().string().latin1().data());

#if PLATFORM(IOS)
    // If there's a web archive resource for this URL, we don't need to schedule the load since it will never touch the network.
    if (!isSuspendingPendingRequests() && resourceLoader->documentLoader()->archiveResourceForURL(resourceLoader->iOSOriginalRequest().url())) {
        resourceLoader->startLoading();
        return;
    }
#else
    if (resourceLoader->documentLoader()->archiveResourceForURL(resourceLoader->request().url())) {
        resourceLoader->start();
        return;
    }
#endif

#if PLATFORM(IOS)
    HostInformation* host = hostForURL(resourceLoader->iOSOriginalRequest().url(), CreateIfNotFound);
#else
    HostInformation* host = hostForURL(resourceLoader->url(), CreateIfNotFound);
#endif

    ResourceLoadPriority priority = resourceLoader->request().priority();

    bool hadRequests = host->hasRequests();
    host->schedule(resourceLoader, priority);

#if PLATFORM(COCOA) || USE(CFNETWORK)
    if (ResourceRequest::resourcePrioritiesEnabled() && !isSuspendingPendingRequests()) {
        // Serve all requests at once to keep the pipeline full at the network layer.
        // FIXME: Does this code do anything useful, given that we also set maxRequestsInFlightPerHost to effectively unlimited on these platforms?
        servePendingRequests(host, ResourceLoadPriority::VeryLow);
        return;
    }
#endif

#if PLATFORM(IOS)
    if ((priority > ResourceLoadPriority::Low || !resourceLoader->iOSOriginalRequest().url().protocolIsInHTTPFamily() || (priority == ResourceLoadPriority::Low && !hadRequests)) && !isSuspendingPendingRequests()) {
        // Try to request important resources immediately.
        servePendingRequests(host, priority);
        return;
    }
#else
    if (priority > ResourceLoadPriority::Low || !resourceLoader->url().protocolIsInHTTPFamily() || (priority == ResourceLoadPriority::Low && !hadRequests)) {
        // Try to request important resources immediately.
        servePendingRequests(host, priority);
        return;
    }
#endif

    // Handle asynchronously so early low priority requests don't
    // get scheduled before later high priority ones.
    scheduleServePendingRequests();
}

#if USE(QUICK_LOOK)
bool ResourceLoadScheduler::maybeLoadQuickLookResource(ResourceLoader& loader)
{
    if (!loader.request().url().protocolIs(QLPreviewProtocol()))
        return false;

    loader.start();
    return true;
}
#endif

void ResourceLoadScheduler::remove(ResourceLoader* resourceLoader)
{
    ASSERT(resourceLoader);

    HostInformation* host = hostForURL(resourceLoader->url());
    if (host)
        host->remove(resourceLoader);
#if PLATFORM(IOS)
    // ResourceLoader::url() doesn't start returning the correct value until the load starts. If we get canceled before that, we need to look for originalRequest url instead.
    // FIXME: ResourceLoader::url() should be made to return a sensible value at all times.
    if (!resourceLoader->iOSOriginalRequest().isNull()) {
        HostInformation* originalHost = hostForURL(resourceLoader->iOSOriginalRequest().url());
        if (originalHost && originalHost != host)
            originalHost->remove(resourceLoader);
    }
#endif
    scheduleServePendingRequests();
}

void ResourceLoadScheduler::setDefersLoading(ResourceLoader*, bool)
{
}

void ResourceLoadScheduler::crossOriginRedirectReceived(ResourceLoader* resourceLoader, const URL& redirectURL)
{
    HostInformation* oldHost = hostForURL(resourceLoader->url());
    ASSERT(oldHost);
    if (!oldHost)
        return;

    HostInformation* newHost = hostForURL(redirectURL, CreateIfNotFound);

    if (oldHost->name() == newHost->name())
        return;

    newHost->addLoadInProgress(resourceLoader);
    oldHost->remove(resourceLoader);
}

void ResourceLoadScheduler::servePendingRequests(ResourceLoadPriority minimumPriority)
{
    LOG(ResourceLoading, "ResourceLoadScheduler::servePendingRequests. m_suspendPendingRequestsCount=%d", m_suspendPendingRequestsCount); 
    if (isSuspendingPendingRequests())
        return;

    m_requestTimer.stop();
    
    servePendingRequests(m_nonHTTPProtocolHost, minimumPriority);

    Vector<HostInformation*> hostsToServe;
    copyValuesToVector(m_hosts, hostsToServe);

    for (auto* host : hostsToServe) {
        if (host->hasRequests())
            servePendingRequests(host, minimumPriority);
        else
            delete m_hosts.take(host->name());
    }
}

void ResourceLoadScheduler::servePendingRequests(HostInformation* host, ResourceLoadPriority minimumPriority)
{
    LOG(ResourceLoading, "ResourceLoadScheduler::servePendingRequests HostInformation.m_name='%s'", host->name().latin1().data());

    auto priority = ResourceLoadPriority::Highest;
    while (true) {
        auto& requestsPending = host->requestsPending(priority);
        while (!requestsPending.isEmpty()) {
            RefPtr<ResourceLoader> resourceLoader = requestsPending.first();

            // For named hosts - which are only http(s) hosts - we should always enforce the connection limit.
            // For non-named hosts - everything but http(s) - we should only enforce the limit if the document isn't done parsing 
            // and we don't know all stylesheets yet.
            Document* document = resourceLoader->frameLoader() ? resourceLoader->frameLoader()->frame().document() : 0;
            bool shouldLimitRequests = !host->name().isNull() || (document && (document->parsing() || !document->haveStylesheetsLoaded()));
            if (shouldLimitRequests && host->limitRequests(priority))
                return;

            requestsPending.removeFirst();
            host->addLoadInProgress(resourceLoader.get());
#if PLATFORM(IOS)
            if (!applicationIsWebProcess()) {
                resourceLoader->startLoading();
                return;
            }
#endif
            resourceLoader->start();
        }
        if (priority == minimumPriority)
            return;
        --priority;
    }
}

void ResourceLoadScheduler::suspendPendingRequests()
{
    ++m_suspendPendingRequestsCount;
}

void ResourceLoadScheduler::resumePendingRequests()
{
    ASSERT(m_suspendPendingRequestsCount);
    --m_suspendPendingRequestsCount;
    if (m_suspendPendingRequestsCount)
        return;
    if (!m_hosts.isEmpty() || m_nonHTTPProtocolHost->hasRequests())
        scheduleServePendingRequests();
}
    
void ResourceLoadScheduler::scheduleServePendingRequests()
{
    LOG(ResourceLoading, "ResourceLoadScheduler::scheduleServePendingRequests, m_requestTimer.isActive()=%u", m_requestTimer.isActive());
    if (!m_requestTimer.isActive())
        m_requestTimer.startOneShot(0);
}

void ResourceLoadScheduler::requestTimerFired()
{
    LOG(ResourceLoading, "ResourceLoadScheduler::requestTimerFired\n");
    servePendingRequests();
}

ResourceLoadScheduler::HostInformation::HostInformation(const String& name, unsigned maxRequestsInFlight)
    : m_name(name)
    , m_maxRequestsInFlight(maxRequestsInFlight)
{
}

ResourceLoadScheduler::HostInformation::~HostInformation()
{
    ASSERT(!hasRequests());
}

unsigned ResourceLoadScheduler::HostInformation::priorityToIndex(ResourceLoadPriority priority)
{
    switch (priority) {
    case ResourceLoadPriority::VeryLow:
        return 0;
    case ResourceLoadPriority::Low:
        return 1;
    case ResourceLoadPriority::Medium:
        return 2;
    case ResourceLoadPriority::High:
        return 3;
    case ResourceLoadPriority::VeryHigh:
        return 4;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

void ResourceLoadScheduler::HostInformation::schedule(ResourceLoader* resourceLoader, ResourceLoadPriority priority)
{
    m_requestsPending[priorityToIndex(priority)].append(resourceLoader);
}
    
void ResourceLoadScheduler::HostInformation::addLoadInProgress(ResourceLoader* resourceLoader)
{
    LOG(ResourceLoading, "HostInformation '%s' loading '%s'. Current count %d", m_name.latin1().data(), resourceLoader->url().string().latin1().data(), m_requestsLoading.size());
    m_requestsLoading.add(resourceLoader);
}
    
void ResourceLoadScheduler::HostInformation::remove(ResourceLoader* resourceLoader)
{
    if (m_requestsLoading.remove(resourceLoader))
        return;
    
    for (auto& requestQueue : m_requestsPending) {
        for (auto it = requestQueue.begin(), end = requestQueue.end(); it != end; ++it) {
            if (*it == resourceLoader) {
                requestQueue.remove(it);
                return;
            }
        }
    }
}

bool ResourceLoadScheduler::HostInformation::hasRequests() const
{
    if (!m_requestsLoading.isEmpty())
        return true;
    for (auto& requestQueue : m_requestsPending) {
        if (!requestQueue.isEmpty())
            return true;
    }
    return false;
}

bool ResourceLoadScheduler::HostInformation::limitRequests(ResourceLoadPriority priority) const 
{
    if (priority == ResourceLoadPriority::VeryLow && !m_requestsLoading.isEmpty())
        return true;
    return m_requestsLoading.size() >= (resourceLoadScheduler()->isSerialLoadingEnabled() ? 1 : m_maxRequestsInFlight);
}

} // namespace WebCore
