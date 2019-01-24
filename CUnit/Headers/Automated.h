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
 *  Automated Interface (generates HTML Report Files).
 *
 *  Feb 2002      Initial implementation (AK)
 *
 *  13-Feb-2002   Single interface to automated_run_tests. (AK)
 *
 *  20-Jul-2004   New interface, doxygen comments. (JDS)
 *
 *  24-Jan-2019   Added common interface for different reports formats.  (PMi)
 */

/** @file
 * Automated testing interface with xml output (user interface).
 */
/** @addtogroup Automated
 * @{
 */

#ifndef CUNIT_AUTOMATED_H_SEEN
#define CUNIT_AUTOMATED_H_SEEN

#include "CUnit.h"
#include "TestDB.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*CU_report_set_output_filename_T)(const char*);
typedef CU_ErrorCode (*CU_report_open_report_T)(void);
typedef CU_ErrorCode(*CU_report_close_report_T)(void);
typedef void (*CU_report_test_start_msg_handler_T)(const CU_pTest, const CU_pSuite);
typedef void (*CU_report_test_complete_msg_handler_T)(const CU_pTest, const CU_pSuite, const CU_pFailureRecord);
typedef void (*CU_report_all_tests_complete_msg_handler_T)(const CU_pFailureRecord);
typedef void (*CU_report_suite_init_failure_msg_handler_T)(const CU_pSuite);
typedef void (*CU_report_suite_cleanup_failure_msg_handler_T)(const CU_pSuite);
typedef CU_ErrorCode (*CU_report_list_all_tests_T)(CU_pTestRegistry);
/**<
 *  Types for each interface functions of report.
 */

typedef struct CU_reportFormat_Tag
{
  CU_report_set_output_filename_T pSetOutputFilename;
  CU_report_open_report_T pOpenReport;
  CU_report_close_report_T pCloseReport;
  CU_report_test_start_msg_handler_T pTestStartMsgHandler;
  CU_report_test_complete_msg_handler_T pTestCompleteMsgHandler;
  CU_report_all_tests_complete_msg_handler_T pAllTestsCompleteMsgHandler;
  CU_report_suite_init_failure_msg_handler_T pSuiteInitFailureMsgHandler;
  CU_report_suite_cleanup_failure_msg_handler_T pSuiteCleanupFailureMsgHandler;
  CU_report_list_all_tests_T pListAllTests;
} CU_reportFormat_T;
typedef CU_reportFormat_T* CU_pReportFormat_T;
/**<
 *  Type for structure with interface of report.
 */

CU_EXPORT void CU_automated_run_tests(void);
/**<
 *  Runs CUnit tests using the automated interface.
 *  This function sets appropriate callback functions, initializes the
 *  test output files, and calls the appropriate functions to list the
 *  tests and run them.  If an output file name root has not been
 *  specified using CU_set_output_filename(), a generic root will be
 *  applied.  It is an error to call this function before the CUnit
 *  test registry has been initialized (check by assertion).
 */

CU_EXPORT CU_ErrorCode CU_list_tests_to_file(void);
/**<
 *  Generates an xml file containing a list of all tests in all suites
 *  in the active registry.  The output file will be named according to
 *  the most recent call to CU_set_output_filename(), or a default if
 *  not previously set.
 *
 *  @return An error code indicating the error status.
 */

CU_EXPORT void CU_set_output_filename(const char* szFilenameRoot);
/**<
 *  Sets the root file name for automated test output files.
 *  The strings "-Listing.xml" and "-Results.xml" are appended to the
 *  specified root to generate the filenames.  If szFilenameRoot is
 *  empty, the default root ("CUnitAutomated") is used.
 *
 *  @param szFilenameRoot String containing root to use for file names.
 */

#ifdef USE_DEPRECATED_CUNIT_NAMES
/** Deprecated (version 1). @deprecated Use CU_automated_run_tests(). */
#define automated_run_tests() CU_automated_run_tests()
/** Deprecated (version 1). @deprecated Use CU_set_output_filename(). */
#define set_output_filename(x) CU_set_output_filename((x))
#endif  /* USE_DEPRECATED_CUNIT_NAMES */

CU_EXPORT void CU_automated_package_name_set(const char *pName);

CU_EXPORT const char *CU_automated_package_name_get();

CU_EXPORT void CU_automated_set_report_format(CU_pReportFormat_T pReportFormat);
/**<
 *  Selects specific report formatter.
 */
#ifdef __cplusplus
}
#endif
#endif  /*  CUNIT_AUTOMATED_H_SEEN  */
/** @} */
