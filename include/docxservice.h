#pragma once

#ifndef METERINGACTS_DOCXSERVICE_H
#define METERINGACTS_DOCXSERVICE_H

#include "actdata.h"

class QWidget;

class DocxService
{
public:
    static bool generate(
        const ActData &data,
        QWidget *parent = nullptr);

};



#endif //METERINGACTS_DOCXSERVICE_H










