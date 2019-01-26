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
  *  JUnit Format interface (generates JUnit XML Report Files).
  *
  *  24-Jan-2019   Initial implementation (PMi)
  */

  /** @file
   * Automated testing interface with xml output (user interface).
   */
   /** @addtogroup Automated
    * @{
    */

#ifndef CUNIT_REPORT_JUNIT_H_SEEN
#define CUNIT_REPORT_JUNIT_H_SEEN

#include "CUnit.h"
#include "Automated.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CU_REPORT_FORMAT_JUNIT (&CU_reportFormat_JUnit)

extern CU_reportFormat_T CU_reportFormat_JUnit;

extern void CU_report_JUnit_set_output_filename(const char* szFilename);
extern CU_ErrorCode CU_report_JUnit_open_report(void);
extern CU_ErrorCode CU_report_JUnit_close_report(void);
extern void CU_report_JUnit_all_tests_complete_msg_handler(const CU_pFailureRecord pFailure);
extern void CU_report_JUnit_suite_complete_msg_handler(const CU_pSuite pSuite, const CU_pFailureRecord pFailure);
#ifdef __cplusplus
}
#endif
#endif  /*  CUNIT_REPORT_JUNIT_H_SEEN  */
/** @} */