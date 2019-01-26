/*
 *  CUnit - A Unit testing framework library for C.
 *  Copyright (C) 2001       Anil Kumar
 *  Copyright (C) 2004-2006  Anil Kumar, Jerry St.Clair
 *  Copyright (C) 2019       Piotr Mis
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

 /*
  *  CUnit Format interface (generates CUnit XML Report Files).
  *
  *  24-Jan-2019   Initial implementation (PMi)
  */

  /** @file
   * Automated testing interface with xml output (user interface).
   */
   /** @addtogroup Automated
    * @{
    */

#ifndef CUNIT_REPORT_CUNIT_H_SEEN
#define CUNIT_REPORT_CUNIT_H_SEEN

#include "CUnit.h"
#include "Automated.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CU_REPORT_FORMAT_CUNIT (&CU_reportFormat_CUnit)

extern CU_reportFormat_T CU_reportFormat_CUnit;

extern void CU_report_CUnit_set_output_filename(const char* szFilename);
extern CU_ErrorCode CU_report_CUnit_open_report(void);
extern CU_ErrorCode CU_report_CUnit_close_report(void);
extern void CU_report_CUnit_test_start_msg_handler(const CU_pTest pTest, const CU_pSuite pSuite);
extern void CU_report_CUnit_test_complete_msg_handler(const CU_pTest pTest, const CU_pSuite pSuite, const CU_pFailureRecord pFailure);
extern void CU_report_CUnit_all_tests_complete_msg_handler(const CU_pFailureRecord pFailure);
extern void CU_report_CUnit_suite_init_failure_msg_handler(const CU_pSuite pSuite);
extern void CU_report_CUnit_suite_cleanup_failure_msg_handler(const CU_pSuite pSuite);
extern CU_ErrorCode CU_report_CUnit_list_all_tests(CU_pTestRegistry pRegistry);

#ifdef __cplusplus
}
#endif
#endif  /*  CUNIT_REPORT_CUNIT_H_SEEN  */
/** @} */