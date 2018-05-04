#pragma once

#include <cstdint>
#include "IWelsVP.h"

namespace WelsVP {

#define MAX_STRATEGY_NUM (METHOD_MASK - 1)

class IStrategy : public IWelsVP {
public:
    IStrategy() {
        m_eMethod = METHOD_NULL;
    }

public:
    EResult Init(int32_t iType, void *pCfg) override {
        return RET_SUCCESS;
    }

    EResult Uninit(int32_t iType) override {
        return RET_SUCCESS;
    }

    EResult Flush(int32_t iType) override {
        return RET_SUCCESS;
    }

    EResult Get(int32_t iType, void *pParam) override {
        return RET_SUCCESS;
    }

    EResult Set(int32_t iType, void *pParam) override {
        return RET_SUCCESS;
    }

    EResult SpecialFeature(int32_t iType, void *pIn, void *pOut) override {
        return RET_SUCCESS;
    }

    EResult Process(int32_t iType, SPixMap *pSrc, SPixMap *pDst) override = 0;

public:
    EMethods m_eMethod;
};

}
