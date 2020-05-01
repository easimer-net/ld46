// === Copyright (c) 2020 easimer.net. All rights reserved. ===
//
// Purpose: output abstraction
//

#pragma once

class IOutput {
public:
    virtual void Release() = 0;
    virtual void Printf(char const* pszFormat, ...) = 0;
};

IOutput* OutputToFile(char const* pszPath);
