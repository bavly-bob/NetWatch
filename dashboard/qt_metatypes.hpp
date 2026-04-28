#pragma once
// qt_metatypes.hpp
// Include this ONLY from the Dashboard (Qt) target.
// Registers the shared data structs with Qt's type system so they
// can be passed across thread boundaries via Qt signals/slots.

#include "message_types.hpp"
#include <QMetaType>

Q_DECLARE_METATYPE(ProcessInfo)
Q_DECLARE_METATYPE(SystemStats)
