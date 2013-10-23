#include "arch/runtime/coroutines.hpp"
#include "arch/timing.hpp"
#include "buffer_cache/alt/page.hpp"
#include "concurrency/auto_drainer.hpp"
#include "serializer/config.hpp"
#include "unittest/gtest.hpp"
#include "unittest/mock_file.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

using alt::alt_access_t;
using alt::current_page_acq_t;
using alt::current_page_t;
using alt::page_acq_t;
using alt::page_cache_t;
using alt::page_t;
using alt::page_txn_t;

struct mock_ser_t {
    mock_file_opener_t opener;
    scoped_ptr_t<standard_serializer_t> ser;

    mock_ser_t()
        : opener() {
        standard_serializer_t::create(&opener,
                                      standard_serializer_t::static_config_t());
        ser = make_scoped<standard_serializer_t>(log_serializer_t::dynamic_config_t(),
                                                 &opener,
                                                 &get_global_perfmon_collection());
    }
};

void run_Control() {
    mock_ser_t ser;
}

TEST(PageTest, Control) {
    run_in_thread_pool(run_Control, 4);
}

void run_CreateDestroy() {
    mock_ser_t mock;
    page_cache_t page_cache(mock.ser.get());
}

TEST(PageTest, CreateDestroy) {
    run_in_thread_pool(run_CreateDestroy, 4);
}

void run_OneTxn() {
    mock_ser_t mock;
    page_cache_t page_cache(mock.ser.get());
    page_txn_t txn(&page_cache);
}

TEST(PageTest, OneTxn) {
    run_in_thread_pool(run_OneTxn, 4);
}

void run_TwoIndependentTxn() {
    mock_ser_t mock;
    page_cache_t page_cache(mock.ser.get());
    page_txn_t txn1(&page_cache);
    page_txn_t txn2(&page_cache);
}

TEST(PageTest, TwoIndependentTxn) {
    run_in_thread_pool(run_TwoIndependentTxn, 4);
}

void run_TwoIndependentTxnSwitch() {
    mock_ser_t mock;
    page_cache_t page_cache(mock.ser.get());
    auto txn1 = make_scoped<page_txn_t>(&page_cache);
    page_txn_t txn2(&page_cache);
    txn1.reset();
}

TEST(PageTest, TwoIndependentTxnSwitch) {
    run_in_thread_pool(run_TwoIndependentTxnSwitch, 4);
}

void run_TwoSequentialTxnSwitch() {
    mock_ser_t mock;
    page_cache_t page_cache(mock.ser.get());
    auto txn1 = make_scoped<page_txn_t>(&page_cache);
    page_txn_t txn2(&page_cache, txn1.get());
    txn1.reset();
}

TEST(PageTest, TwoSequentialTxnSwitch) {
    run_in_thread_pool(run_TwoSequentialTxnSwitch, 4);
}

void run_OneReadAcq() {
    mock_ser_t mock;
    page_cache_t page_cache(mock.ser.get());
    page_txn_t txn(&page_cache);
    current_page_acq_t acq(&txn, 0, alt_access_t::read);
    // Do nothing with the acq.
}

TEST(PageTest, OneReadAcq) {
    run_in_thread_pool(run_OneReadAcq, 4);
}

void run_OneWriteAcq() {
    mock_ser_t mock;
    page_cache_t page_cache(mock.ser.get());
    page_txn_t txn(&page_cache);
    current_page_acq_t acq(&txn, 0, alt_access_t::write);
    // Do nothing with the acq.
}

TEST(PageTest, OneWriteAcq) {
    run_in_thread_pool(run_OneWriteAcq, 4);
}

void run_OneWriteAcqWait() {
    mock_ser_t mock;
    page_cache_t page_cache(mock.ser.get());
    page_txn_t txn(&page_cache);
    current_page_acq_t acq(&txn, alt_access_t::write);
    page_acq_t page_acq;
    page_t *page = acq.current_page_for_write();
    page_acq.init(page);
    ASSERT_TRUE(page_acq.buf_ready_signal()->is_pulsed());
    void *buf = page_acq.get_buf_write();
    ASSERT_TRUE(buf != NULL);
}

TEST(PageTest, OneWriteAcqWait) {
    run_in_thread_pool(run_OneWriteAcqWait, 4);
}

// RSI: motherfucker
#define spawn_ordered (coro_t::spawn_later_ordered)
#define R (alt_access_t::read)
#define W (alt_access_t::write)


class bigger_test_t {
public:
    bigger_test_t() : mock(), c(mock.ser.get()) {
        for (size_t i = 0; i < b_len; ++i) {
            b[i] = NULL_BLOCK_ID;
        }
    }

    void run() {
        spawn_ordered(std::bind(&bigger_test_t::run_txn6, this, drainer.lock()));
        spawn_ordered(std::bind(&bigger_test_t::run_txn7, this, drainer.lock()));
        spawn_ordered(std::bind(&bigger_test_t::run_txn8, this, drainer.lock()));

        t678cond.pulse();
        // RSI: if 0
#if 0
        spawn_ordered(std::bind(&bigger_test_t::run_txn1, this, drainer.lock()));
        spawn_ordered(std::bind(&bigger_test_t::run_txn2, this, drainer.lock()));
        spawn_ordered(std::bind(&bigger_test_t::run_txn3, this, drainer.lock()));
        spawn_ordered(std::bind(&bigger_test_t::run_txn4, this, drainer.lock()));
        spawn_ordered(std::bind(&bigger_test_t::run_txn5, this, drainer.lock()));
        spawn_ordered(std::bind(&bigger_test_t::run_txn9, this, drainer.lock()));
        spawn_ordered(std::bind(&bigger_test_t::run_txn10, this, drainer.lock()));
        spawn_ordered(std::bind(&bigger_test_t::run_txn11, this, drainer.lock()));
        spawn_ordered(std::bind(&bigger_test_t::run_txn12, this, drainer.lock()));
        spawn_ordered(std::bind(&bigger_test_t::run_txn13, this, drainer.lock()));
        spawn_ordered(std::bind(&bigger_test_t::run_txn14, this, drainer.lock()));
#endif
    }

private:
    void run_txn6(auto_drainer_t::lock_t) {
        page_txn_t txn6(&c);
        auto acq12 = make_scoped<current_page_acq_t>(&txn6, W);
        ASSERT_EQ(NULL_BLOCK_ID, b[12]);
        b[12] = acq12->block_id();
        acq12->write_acq_signal()->wait();
        auto acq13 = make_scoped<current_page_acq_t>(&txn6, W);
        acq12.reset();
        ASSERT_EQ(NULL_BLOCK_ID, b[13]);
        b[13] = acq13->block_id();
        acq13->write_acq_signal()->wait();
        auto acq14 = make_scoped<current_page_acq_t>(&txn6, W);
        acq13.reset();
        ASSERT_EQ(NULL_BLOCK_ID, b[14]);
        b[14] = acq14->block_id();
        ASSERT_NE(NULL_BLOCK_ID, b[12]);
        ASSERT_NE(NULL_BLOCK_ID, b[13]);
        ASSERT_NE(NULL_BLOCK_ID, b[14]);
        assert_unique_ids();
        acq14->write_acq_signal()->wait();

        // Wait while holding block.
        t678cond.wait();
        acq14.reset();
    }

    void run_txn7(auto_drainer_t::lock_t) {
        page_txn_t txn7(&c);
        auto acq15 = make_scoped<current_page_acq_t>(&txn7, W);
        ASSERT_EQ(NULL_BLOCK_ID, b[15]);
        b[15] = acq15->block_id();
        acq15->write_acq_signal()->wait();
        auto acq16 = make_scoped<current_page_acq_t>(&txn7, W);
        acq15.reset();
        ASSERT_EQ(NULL_BLOCK_ID, b[16]);
        b[16] = acq16->block_id();
        acq16->write_acq_signal()->wait();
        acq16.reset();

        // Wait after releasing block, to be different than run_txn6.
        t678cond.wait();
    }

    void run_txn8(auto_drainer_t::lock_t) {
        page_txn_t txn8(&c);
        auto acq17 = make_scoped<current_page_acq_t>(&txn8, W);
        ASSERT_EQ(NULL_BLOCK_ID, b[17]);
        b[17] = acq17->block_id();
        acq17->write_acq_signal()->wait();
        acq17.reset();

        // Idk, wait after releasing block.
        t678cond.wait();
    }

    void assert_unique_ids() {
        for (size_t i = 0; i < b_len; ++i) {
            if (b[i] != NULL_BLOCK_ID) {
                for (size_t j = 0; j < b_len; ++j) {
                    if (i != j) {
                        ASSERT_NE(b[i], b[j]);
                    }
                }
            }
        }
    }

    mock_ser_t mock;
    page_cache_t c;

    static const int b_len = 18;
    block_id_t b[b_len];

    cond_t t678cond;

    auto_drainer_t drainer;
};

void run_BiggerTest() {
    bigger_test_t test;
    test.run();
}

TEST(PageTest, BiggerTest) {
    run_in_thread_pool(run_BiggerTest, 4);
}

}  // namespace unittest
