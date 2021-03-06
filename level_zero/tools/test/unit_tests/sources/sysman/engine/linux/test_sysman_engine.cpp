/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "level_zero/core/test/unit_tests/fixtures/device_fixture.h"
#include "level_zero/tools/source/sysman/engine/linux/os_engine_imp.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mock_engine.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

namespace L0 {
namespace ult {

ACTION_P(SetUint64_t, value) {
    arg0 = value;
}
ACTION_P(SetEngGroup_t, value) {
    arg0 = value;
}

ACTION_P(SetString_t, value) {
    arg1 = value;
}

class SysmanEngineFixture : public DeviceFixture, public ::testing::Test {

  protected:
    SysmanImp *sysmanImp;
    zet_sysman_handle_t hSysman;
    zet_sysman_engine_handle_t hSysmanEngine;

    Mock<OsEngine> *pOsEngine;
    EngineImp *pEngineImp;
    const uint64_t activeTime = 2147483648u;

    void SetUp() override {

        DeviceFixture::SetUp();
        sysmanImp = new SysmanImp(device->toHandle());
        pOsEngine = new NiceMock<Mock<OsEngine>>;
        pEngineImp = new EngineImp();
        pEngineImp->pOsEngine = pOsEngine;
        ON_CALL(*pOsEngine, getEngineGroup(_))
            .WillByDefault(DoAll(
                SetEngGroup_t(ZET_ENGINE_GROUP_COMPUTE_ALL),
                Return(ZE_RESULT_SUCCESS)));
        ON_CALL(*pOsEngine, getActiveTime(_))
            .WillByDefault(DoAll(
                SetUint64_t(activeTime),
                Return(ZE_RESULT_SUCCESS)));

        pEngineImp->init();
        sysmanImp->pEngineHandleContext->handleList.push_back(pEngineImp);

        hSysman = sysmanImp->toHandle();
        hSysmanEngine = pEngineImp->toHandle();
    }
    void TearDown() override {
        //pOsEngine will be deleted in pEngineImp destructor and pEngineImp will be deleted by sysmanImp destructor
        if (sysmanImp != nullptr) {
            delete sysmanImp;
            sysmanImp = nullptr;
        }
        DeviceFixture::TearDown();
    }
};

TEST_F(SysmanEngineFixture, GivenComponentCountZeroWhenCallingZetSysmanEngineGetThenNonZeroCountIsReturnedAndVerifySysmanEngineGetCallSucceeds) {
    zet_sysman_engine_handle_t engineHandle;
    uint32_t count = 0;
    ze_result_t result = zetSysmanEngineGet(hSysman, &count, NULL);

    EXPECT_EQ(ZE_RESULT_SUCCESS, result);
    EXPECT_GT(count, 0u);

    uint32_t testcount = count + 1;

    result = zetSysmanEngineGet(hSysman, &testcount, NULL);

    EXPECT_EQ(ZE_RESULT_SUCCESS, result);
    EXPECT_EQ(testcount, count);

    result = zetSysmanEngineGet(hSysman, &count, &engineHandle);

    EXPECT_EQ(ZE_RESULT_SUCCESS, result);
    EXPECT_NE(nullptr, engineHandle);
    EXPECT_GT(count, 0u);
}
TEST_F(SysmanEngineFixture, GivenValidEngineHandleWhenCallingZetSysmanEngineGetPropertiesThenVerifySysmanEngineGetPropertiesCallSucceeds) {
    zet_engine_properties_t properties;

    ze_result_t result = zetSysmanEngineGetProperties(hSysmanEngine, &properties);

    EXPECT_EQ(ZE_RESULT_SUCCESS, result);
    EXPECT_EQ(ZET_ENGINE_GROUP_COMPUTE_ALL, properties.type);
    EXPECT_FALSE(properties.onSubdevice);
}

TEST_F(SysmanEngineFixture, GivenValidEngineHandleWhenCallingZetSysmanGetActivityThenVerifySysmanEngineGetActivityCallSucceeds) {
    zet_engine_stats_t Stats;

    ze_result_t result = zetSysmanEngineGetActivity(hSysmanEngine, &Stats);

    EXPECT_EQ(ZE_RESULT_SUCCESS, result);
    EXPECT_EQ(activeTime, Stats.activeTime);
    EXPECT_GT(Stats.timestamp, 0u);
}

TEST_F(SysmanEngineFixture, whenCalllingGetEngineGroupWithNoValidGroupFileThenResultDifferentToSuccessIsRetured) {
    auto engineImp = std::make_unique<WhiteBox<::L0::LinuxEngineImp>>();

    Mock<SysfsAccess> sysfsAccess;
    engineImp->pSysfsAccess = &sysfsAccess;

    EXPECT_CALL(sysfsAccess, read(_, _))
        .Times(1)
        .WillOnce(Return(ZE_RESULT_ERROR_UNKNOWN));

    zet_engine_group_t engineGroup;
    ze_result_t res = engineImp->getEngineGroup(engineGroup);
    EXPECT_NE(ZE_RESULT_SUCCESS, res);
}

TEST_F(SysmanEngineFixture, whenCalllingGetEngineGroupWithValidGroupFileThenResultSuccessIsRetured) {
    auto engineImp = std::make_unique<WhiteBox<::L0::LinuxEngineImp>>();

    Mock<SysfsAccess> sysfsAccess;
    engineImp->pSysfsAccess = &sysfsAccess;

    zet_engine_group_t engineGroup;
    EXPECT_CALL(sysfsAccess, read(_, _))
        .Times(1)
        .WillOnce(::testing::Invoke(&sysfsAccess, &Mock<SysfsAccess>::doRead));
    ze_result_t res = engineImp->getEngineGroup(engineGroup);
    EXPECT_EQ(ZE_RESULT_SUCCESS, res);
    EXPECT_EQ(ZET_ENGINE_GROUP_COMPUTE_ALL, engineGroup);
}
} // namespace ult
} // namespace L0
