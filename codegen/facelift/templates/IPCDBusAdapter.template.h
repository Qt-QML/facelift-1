{#*********************************************************************
**
** Copyright (C) 2018 Luxoft Sweden AB
**
** This file is part of the FaceLift project
**
** Permission is hereby granted, free of charge, to any person
** obtaining a copy of this software and associated documentation files
** (the "Software"), to deal in the Software without restriction,
** including without limitation the rights to use, copy, modify, merge,
** publish, distribute, sublicense, and/or sell copies of the Software,
** and to permit persons to whom the Software is furnished to do so,
** subject to the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
** BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
** ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
**
** SPDX-License-Identifier: MIT
**
*********************************************************************#}

/****************************************************************************
** This is an auto-generated file.
** Do not edit! All changes made to it will be lost.
****************************************************************************/

#pragma once

{{classExportDefines}}

#include "DBusIPCServiceAdapter.h"
#include "FaceliftUtils.h"

#include "{{module.fullyQualifiedPath}}/{{interfaceName}}.h"
#include "{{module.fullyQualifiedPath}}/{{interfaceName}}QMLFrontend.h"
#include "{{module.fullyQualifiedPath}}/{{interfaceName}}IPCDBus.h"

//// Sub interfaces
{% for property in interface.referencedInterfaceTypes %}
#include "{{property.fullyQualifiedPath}}{% if generateAsyncProxy %}Async{% endif %}IPCDBusAdapter.h"
{% endfor %}

{{module.namespaceCppOpen}}

class {{interfaceName}}IPCQMLFrontendType;

class {{classExport}} {{interfaceName}}IPCDBusAdapter: public ::facelift::dbus::DBusIPCServiceAdapter<{{interfaceName}}>
{
    Q_OBJECT

public:

    using ServiceType = {{interfaceName}};
    using BaseType = ::facelift::dbus::DBusIPCServiceAdapter<{{interfaceName}}>;
    using ThisType = {{interfaceName}}IPCDBusAdapter;
    using SignalID = {{interface}}IPCDBus::SignalID;
    using MethodID = {{interface}}IPCDBus::MethodID;

    {{interfaceName}}IPCDBusAdapter(QObject* parent = nullptr) : BaseType(parent)
    {% for property in interface.properties %}
    {% if property.type.is_model %}
        , m_{{property.name}}Handler(*this)
    {% endif %}
    {% endfor %}
    {
    }

    void appendDBUSIntrospectionData(QTextStream &s) const override;

    ::facelift::IPCHandlingResult handleMethodCallMessage(::facelift::dbus::DBusIPCMessage &requestMessage,
            ::facelift::dbus::DBusIPCMessage &replyMessage) override;

    void connectSignals() override;

    void serializePropertyValues(::facelift::dbus::DBusIPCMessage& msg, bool isCompleteSnapshot) override;

    {% for event in interface.signals %}
    void {{event}}(
    {%- set comma = joiner(", ") -%}
    {%- for parameter in event.parameters -%}
        {{ comma() }}{{parameter.interfaceCppType}} {{parameter.name}}
    {%- endfor -%}  )
    {
        sendSignal(SignalID::{{event}}
        {%- for parameter in event.parameters -%}
            , {{parameter.name}}
        {%- endfor -%}  );
    }
    {% endfor %}

private:
    {% for property in interface.properties %}
    {% if property.type.is_model %}
    ::facelift::IPCAdapterModelPropertyHandler<ThisType, {{property.nestedType.interfaceCppType}}> m_{{property.name}}Handler;
    {% elif property.type.is_interface %}
    QString m_previous{{property.name}}ObjectPath;
    {% else %}
    {{property.interfaceCppType}} m_previous{{property.name}};
    {% endif %}
    {% if property.type.is_interface %}
    InterfacePropertyIPCAdapterHandler<{{property.cppType}}, {{property.cppType}}IPCDBusAdapter> m_{{property.name}};
    {% endif %}
    {% endfor %}
};

{{module.namespaceCppClose}}
