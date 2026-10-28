#pragma once

#include <QString>
#include <QUuid>

#ifndef METERINGACTS_UUIDUTILS_H
#define METERINGACTS_UUIDUTILS_H

namespace UuidUtils
{
    inline QString generate()
    {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
}

#endif //METERINGACTS_UUIDUTILS_H