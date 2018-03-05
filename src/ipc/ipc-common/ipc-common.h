/*
 *   Copyright (C) 2017 Pelagicore AG
 *   SPDX-License-Identifier: LGPL-2.1
 *   This file is subject to the terms of the LGPL 2.1 license.
 *   Please see the LICENSE file for details.
 */

#pragma once

#include <QDebug>

#include "FaceliftModel.h"
#include "QMLFrontend.h"
#include "QMLModel.h"

namespace facelift {

enum class IPCHandlingResult {
    OK,
    INVALID
};

class IPCServiceAdapterBase;


/**
 * This class maintains a registry of IPC services registered locally, which enables local proxies to get a direct reference to them
 */
class InterfaceManager : public QObject
{
    Q_OBJECT

public:
    void registerAdapter(const QString &objectPath, IPCServiceAdapterBase *adapter);

    IPCServiceAdapterBase *getAdapter(const QString &objectPath);

    void onAdapterDestroyed(IPCServiceAdapterBase *object);

    Q_SIGNAL void adapterDestroyed(IPCServiceAdapterBase *adapter);
    Q_SIGNAL void adapterAvailable(IPCServiceAdapterBase *adapter);

    static InterfaceManager &instance();

private:
    QMap<QString, IPCServiceAdapterBase *> m_registry;

};

class IPCServiceAdapterBase : public QObject
{

    Q_OBJECT

public:
    Q_PROPERTY(facelift::InterfaceBase * service READ service WRITE checkedSetService)
    Q_PROPERTY(QString objectPath READ objectPath WRITE setObjectPath)
    Q_PROPERTY(QString interfaceName READ interfaceName WRITE setInterfaceName)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled)

    IPCServiceAdapterBase(QObject *parent = nullptr) : QObject(parent)
    {
    }

    bool enabled() const
    {
        return m_enabled;
    }

    void setEnabled(bool enabled)
    {
        m_enabled = enabled;
        onValueChanged();
    }

    virtual void init() = 0;

    virtual void connectSignals()
    {
    }

    virtual InterfaceBase *service() const = 0;

    void checkedSetService(InterfaceBase *service)
    {
        setService(service);
        onValueChanged();
    }

    virtual void setService(InterfaceBase *service) = 0;

    const QString &objectPath() const
    {
        return m_objectPath;
    }

    void setObjectPath(const QString &objectPath)
    {
        m_objectPath = objectPath;
        onValueChanged();
    }

    const QString &interfaceName() const
    {
        return m_interfaceName;
    }

    void setInterfaceName(const QString &name)
    {
        m_interfaceName = name;
        onValueChanged();
    }

    void onProviderCompleted()
    {
        // The parsing of the provider is finished => all our properties are set and we are ready to register our service
        m_providerReady = true;
        onValueChanged();
    }

    void onValueChanged()
    {
        if (enabled() && m_providerReady && !m_objectPath.isEmpty() && (service() != nullptr)) {
            init();
        }
    }

    void registerLocalService()
    {
        InterfaceManager::instance().registerAdapter(objectPath(), this);
    }

    Q_SIGNAL void destroyed(IPCServiceAdapterBase *adapter);

    template<typename ServiceType>
    ServiceType *toProvider(InterfaceBase *s)
    {
        auto service = qobject_cast<ServiceType *>(s);
        if (service == nullptr) {
            typedef typename ServiceType::QMLFrontendType QMLFrontendType;
            auto *qmlFrontend = qobject_cast<QMLFrontendType *>(service);
            if (qmlFrontend != nullptr) {
                service = qmlFrontend->m_provider;
            } else {
                qFatal("Bad service type : '%s'", qPrintable(facelift::toString(s)));
            }
        }
        Q_ASSERT(service != nullptr);
        QObject::connect(service, &InterfaceBase::componentCompleted, this, &IPCServiceAdapterBase::onProviderCompleted);
        return service;
    }

private:
    QString m_objectPath;
    QString m_interfaceName;
    bool m_enabled = false;

protected:
    bool m_providerReady = false;

};


class IPCProxyBinderBase : public QObject
{
    Q_OBJECT

public:
    Q_PROPERTY(QString objectPath READ objectPath WRITE setObjectPath)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled)

    IPCProxyBinderBase(QObject *parent) : QObject(parent)
    {
    }

    bool enabled() const
    {
        return m_enabled;
    }

    void setEnabled(bool enabled)
    {
        m_enabled = enabled;
        checkInit();
    }

    const QString &objectPath() const
    {
        return m_objectPath;
    }

    void setObjectPath(const QString &objectPath)
    {
        m_objectPath = objectPath;
        checkInit();
    }

    void onComponentCompleted()
    {
        m_componentCompleted = true;
        checkInit();
    }

    void checkInit()
    {
        if (m_componentCompleted && !m_alreadyInitialized && enabled() && !objectPath().isEmpty()) {
            m_alreadyInitialized = true;
            QObject::connect(
                &InterfaceManager::instance(), &InterfaceManager::adapterAvailable, this,
                &IPCProxyBinderBase::onLocalAdapterAvailable);

            auto localAdapter = InterfaceManager::instance().getAdapter(this->objectPath());
            if (localAdapter != nullptr) {
                onLocalAdapterAvailable(localAdapter);
            }

            bindToIPC();
        }
    }

    virtual void bindToIPC() = 0;

    Q_SIGNAL void localAdapterAvailable(IPCServiceAdapterBase *adapter);

    void onLocalAdapterAvailable(IPCServiceAdapterBase *adapter);

private:
    QString m_objectPath;
    bool m_enabled = true;
    bool m_alreadyInitialized = false;
    bool m_componentCompleted = false;


};


template<typename AdapterType, typename IPCAdapterType>
class IPCProxyBase : public AdapterType
{

public:
    typedef typename IPCAdapterType::TheServiceType InterfaceType;

public:
    IPCProxyBase(QObject *parent) : AdapterType(parent)
    {
        m_serviceReady.init("ready", this, &InterfaceBase::readyChanged);
    }

    template<typename BinderType>
    void initBinder(BinderType &binder)
    {
        QObject::connect(&binder, &IPCProxyBinderBase::localAdapterAvailable, this, &IPCProxyBase::onLocalAdapterAvailable);
        QObject::connect(this, &InterfaceBase::componentCompleted, &binder, &BinderType::onComponentCompleted);
    }

    bool ready() const override final
    {
        if (localInterface() != nullptr) {
            return localInterface()->ready();
        }
        return m_serviceReady;
    }

    InterfaceType *localInterface() const
    {
        if (m_localAdapter) {
            return m_localAdapter->service();
        } else {
            return nullptr;
        }
    }

    void onLocalAdapterAvailable(IPCServiceAdapterBase *a)
    {
        auto adapter = qobject_cast<IPCAdapterType *>(a);
        if (adapter != nullptr) {
            m_localAdapter = adapter;
        }

        if (localInterface() != nullptr) {
            bindLocalService(localInterface());
            this->setReady(true);
        }
    }

    virtual void bindLocalService(InterfaceType *service) = 0;

protected:
    void setReady(bool isReady)
    {
        m_serviceReady = isReady;
    }

    QPointer<IPCAdapterType> m_localAdapter;
    Property<bool> m_serviceReady;

};


class IPCAttachedPropertyFactoryBase : public QObject
{

    Q_OBJECT

public:
    IPCAttachedPropertyFactoryBase(QObject *parent) : QObject(parent)
    {
    }

    static IPCServiceAdapterBase *qmlAttachedProperties(QObject *object);

    static InterfaceBase *getProvider(QObject *object)
    {
        InterfaceBase *provider = nullptr;

        auto o = qobject_cast<facelift::QMLFrontendBase *>(object);
        if (o == nullptr) {
            auto qmlImpl = qobject_cast<facelift::ModelQMLImplementationBase *>(object);
            if (qmlImpl != nullptr) {
                provider = qmlImpl->interfac();
            }
        } else {
            provider = o->provider();
        }

        return provider;
    }

};

}
