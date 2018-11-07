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

#include "{{interfaceName}}IPCProxy.h"

{{module.namespaceCppOpen}}


void {{interfaceName}}IPCProxy::deserializePropertyValues(facelift::IPCMessage &msg, bool isCompleteSnapshot)
{
    {% for property in interface.properties %}
    {% if property.type.is_interface %}
    QString {{property.name}}_objectPath;
    if (deserializeOptionalValue(msg, {{property.name}}_objectPath, isCompleteSnapshot))
    {
        m_{{property.name}}Proxy.update({{property.name}}_objectPath);
        m_{{property.name}} = m_{{property.name}}Proxy.getValue();
    }
    {% elif property.type.is_model %}
    if (isCompleteSnapshot) {
        int {{property.name}}Size;
        deserializeValue(msg, {{property.name}}Size);
        m_{{property.name}}.beginResetModel();
        m_{{property.name}}.reset({{property.name}}Size, std::bind(&ThisType::{{property.name}}Data, this, std::placeholders::_1));
        m_{{property.name}}.endResetModel();
    }
    {% else %}
    deserializeOptionalValue(msg, m_{{property.name}}, isCompleteSnapshot);
    {% endif %}
    {% endfor %}
    BaseType::deserializePropertyValues(msg, isCompleteSnapshot);
}

void {{interfaceName}}IPCProxy::emitChangeSignals() {
{% for property in interface.properties %}
    emit {{property.name}}Changed();
{% endfor %}
    BaseType::emitChangeSignals();
}

void {{interfaceName}}IPCProxy::deserializeSignal(facelift::IPCMessage &msg)
{
    SignalID member;
    deserializeValue(msg, member);

    switch (member) {
    {% for event in interface.signals %}
    case SignalID::{{event}}: {
        {% for parameter in event.parameters %}
        {{parameter.interfaceCppType}} param_{{parameter.name}};
        deserializeValue(msg, param_{{parameter.name}});
        {% endfor %}
        emit {{event}}(
        {%- set comma = joiner(", ") -%}
        {%- for parameter in event.parameters -%}
            {{ comma() }}param_{{parameter.name}}
        {%- endfor -%}  );
    }    break;

    {% endfor %}
    {% for property in interface.properties %}
    case SignalID::{{property.name}}:
    {% if property.type.is_model %}
        m_{{property.name}}.handleSignal(msg);
    {% else %}
        emit {{property.name}}Changed();
    {% endif %}
        break;
    {% endfor %}
    default :
        BaseType::deserializeCommonSignal(static_cast<facelift::CommonSignalID>(member));
        break;
    }
}

void {{interfaceName}}IPCProxy::bindLocalService({{interfaceName}} *service)
{
    Q_UNUSED(service);

    // Bind all properties
    {% for property in interface.properties %}
    QObject::connect(service, &{{interfaceName}}::{{property.name}}Changed, this, &{{interfaceName}}::{{property.name}}Changed);
    {% endfor %}

    // Forward all signals
    {% for signal in interface.signals %}
    QObject::connect(service, &InterfaceType::{{signal.name}}, this, &InterfaceType::{{signal.name}});
    {% endfor %}
}

{{module.namespaceCppClose}}