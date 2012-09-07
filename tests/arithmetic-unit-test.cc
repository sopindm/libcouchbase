/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2012 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */
#include "config.h"
#include <gtest/gtest.h>
#include <libcouchbase/couchbase.h>

#include "server.h"
#include "mock-unit-test.h"

class ArithmeticUnitTest : public MockUnitTest
{
protected:
    static void SetUpTestCas() {
        MockUnitTest::SetUpTestCase();
    }
};

static lcb_uint64_t arithm_val;

extern "C" {
    static void arithmetic_store_callback(lcb_t, const void *,
                                          lcb_storage_t operation,
                                          lcb_error_t error,
                                          const lcb_store_resp_t *resp)
    {
        ASSERT_EQ(LCB_SUCCESS, error);
        ASSERT_EQ(LCB_SET, operation);
        ASSERT_EQ(7, resp->v.v0.nkey);
        ASSERT_EQ(0, memcmp(resp->v.v0.key, "counter", 7));
    }

    static void arithmetic_incr_callback(lcb_t, const void *,
                                         lcb_error_t error,
                                         const lcb_arithmetic_resp_t *resp)
    {
        ASSERT_EQ(LCB_SUCCESS, error);
        ASSERT_EQ(7, resp->v.v0.nkey);
        ASSERT_EQ(0, memcmp(resp->v.v0.key, "counter", 7));
        ASSERT_EQ(arithm_val + 1, resp->v.v0.value);
        arithm_val = resp->v.v0.value;
    }

    static void arithmetic_decr_callback(lcb_t, const void *,
                                         lcb_error_t error,
                                         const lcb_arithmetic_resp_t *resp)
    {
        ASSERT_EQ(LCB_SUCCESS, error);
        ASSERT_EQ(7, resp->v.v0.nkey);
        ASSERT_EQ(0, memcmp(resp->v.v0.key, "counter", 7));
        ASSERT_EQ(arithm_val - 1, resp->v.v0.value);
        arithm_val = resp->v.v0.value;
    }

    static void arithmetic_create_callback(lcb_t, const void *,
                                           lcb_error_t error,
                                           const lcb_arithmetic_resp_t *resp)
    {
        ASSERT_EQ(LCB_SUCCESS, error);
        ASSERT_EQ(9, resp->v.v0.nkey);
        ASSERT_EQ(0, memcmp(resp->v.v0.key, "mycounter", 9));
        ASSERT_EQ(0xdeadbeef, resp->v.v0.value);
    }
}

TEST_F(ArithmeticUnitTest, populateArithmetic)
{
    lcb_t instance;
    createConnection(instance);
    (void)lcb_set_store_callback(instance, arithmetic_store_callback);

    lcb_store_cmd_t cmd(LCB_SET, "counter", 7, "0", 1);
    lcb_store_cmd_t *cmds[] = { &cmd };

    lcb_store(instance, this, 1, cmds);
    lcb_wait(instance);
    lcb_destroy(instance);
}

TEST_F(ArithmeticUnitTest, testIncr)
{
    lcb_t instance;
    createConnection(instance);
    (void)lcb_set_arithmetic_callback(instance, arithmetic_incr_callback);

    for (int ii = 0; ii < 10; ++ii) {
        lcb_arithmetic_cmd_t cmd("counter", 7, 1);
        lcb_arithmetic_cmd_t *cmds[] = { &cmd };
        lcb_arithmetic(instance, NULL, 1, cmds);
        lcb_wait(instance);
    }

    lcb_destroy(instance);
}

TEST_F(ArithmeticUnitTest, testDecr)
{
    lcb_t instance;
    createConnection(instance);
    (void)lcb_set_arithmetic_callback(instance, arithmetic_decr_callback);

    for (int ii = 0; ii < 10; ++ii) {
        lcb_arithmetic_cmd_t cmd("counter", 7, -1);
        lcb_arithmetic_cmd_t *cmds[] = { &cmd };
        lcb_arithmetic(instance, NULL, 1, cmds);
        lcb_wait(instance);
    }

    lcb_destroy(instance);
}

TEST_F(ArithmeticUnitTest, testArithmeticCreate)
{
    lcb_t instance;
    createConnection(instance);
    removeKey(instance, "mycounter");
    (void)lcb_set_arithmetic_callback(instance, arithmetic_create_callback);
    lcb_arithmetic_cmd_t cmd("mycounter", 9, 0x77, 1, 0xdeadbeef);
    lcb_arithmetic_cmd_t *cmds[] = { &cmd };
    lcb_arithmetic(instance, NULL, 1, cmds);
    lcb_wait(instance);
    lcb_destroy(instance);
}
