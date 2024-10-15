/*
 * test_scheduler.h
 *
 *  Created on: Dec 11, 2023
 *      Author: Mohamed Raafat
 */

#ifndef KERN_TESTS_TEST_SCHEDULER_H_
#define KERN_TESTS_TEST_SCHEDULER_H_

#ifndef FOS_KERNEL
#error "This is a FOS kernel header; user programs should not #include it"
#endif

void test_bsd_nice_0();
void test_bsd_nice_1();
void test_bsd_nice_2();

#endif
