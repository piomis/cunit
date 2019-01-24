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
 *  Implementation of the Automated Test Interface.
 *
 *  Feb 2002      Initial implementation. (AK)
 *
 *  13/Feb/2002   Added initial automated interface functions to generate
 *                HTML based Run report. (AK)
 *
 *  23/Jul/2002   Changed HTML to XML Format file generation for Automated Tests. (AK)
 *
 *  27/Jul/2003   Fixed a bug which hinders the listing of all failures. (AK)
 *
 *  17-Jul-2004   New interface, doxygen comments, eliminate compiler warnings,
 *                automated_run_tests now assigns a generic file name if
 *                none has been supplied. (JDS)
 *
 *  30-Apr-2005   Added notification of failed suite cleanup function. (JDS)
 *
 *  02-May-2006   Added internationalization hooks.  (JDS)
 *
 *  07-May-2011   Added patch to fix broken xml tags dur to spacial characters in the test name.  (AK)
 *
 *  24-Jan-2019   Refactored and added common interface for different reports formats.  (PMi)
 */

/** @file
 * Automated test interface with xml result output (implementation).
 */
/** @addtogroup Automated
 @{
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#include "CUnit.h"
#include "TestDB.h"
#include "MyMem.h"
#include "Util.h"
#include "TestRun.h"
#include "Automated.h"
#include "CUnit_intl.h"

/*=================================================================
 *  Global / Static data definitions
 *=================================================================*/
static CU_pReportFormat_T pReport;                          /**< selected report formatter. */


static CU_BOOL   bJUnitXmlOutput = CU_FALSE;                /**< Flag for toggling the xml junit output or keeping the original. Off is the default */
static char _gPackageName[50] = "";

/*=================================================================
 *  Static function forward declarations
 *=================================================================*/
static CU_ErrorCode automated_list_all_tests(CU_pTestRegistry pRegistry, const char* szFilename);

static void automated_run_all_tests(CU_pTestRegistry pRegistry);

/*=================================================================
 *  Public Interface functions
 *=================================================================*/

void CU_automated_set_report_format(CU_pReportFormat_T pReportFormat)
{
  assert(NULL != pReportFormat);
  pReport = pReportFormat;
}

void CU_automated_run_tests(void)
{
  assert(NULL != CU_get_registry());
  assert(NULL != pReport);
  assert(NULL != pReport->pOpenReport);
  assert(NULL != pReport->pCloseReport);
  assert(NULL != pReport->pTestStartMsgHandler);
  assert(NULL != pReport->pTestCompleteMsgHandler);
  assert(NULL != pReport->pAllTestsCompleteMsgHandler);
  assert(NULL != pReport->pSuiteInitFailureMsgHandler);
  assert(NULL != pReport->pSuiteCleanupFailureMsgHandler);


  /* Ensure output makes it to screen at the moment of a SIGSEGV. */
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

 if (CUE_SUCCESS != pReport->pOpenReport()) {
    fprintf(stderr, "\n%s", _("ERROR - Failed to create/initialize the result file."));
  }
  else {
    /* set up the message handlers for writing xml output */
    CU_set_test_start_handler(pReport->pTestStartMsgHandler);
    CU_set_test_complete_handler(pReport->pTestCompleteMsgHandler);
    CU_set_all_test_complete_handler(pReport->pAllTestsCompleteMsgHandler);
    CU_set_suite_init_failure_handler(pReport->pSuiteInitFailureMsgHandler);
    CU_set_suite_cleanup_failure_handler(pReport->pSuiteCleanupFailureMsgHandler);

    automated_run_all_tests(NULL);

    if (CUE_SUCCESS != pReport->pCloseReport()) {
      fprintf(stderr, "\n%s", _("ERROR - Failed to close/uninitialize the result files."));
    }
  }
}

/*------------------------------------------------------------------------*/
void CU_set_output_filename(const char* szFilenameRoot)
{
  assert(NULL != pReport);
  assert(NULL != pReport->pSetOutputFilename);

  pReport->pSetOutputFilename(szFilenameRoot);
}

/*------------------------------------------------------------------------*/
CU_ErrorCode CU_list_tests_to_file()
{
  assert(pReport);

  if (pReport->pListAllTests != NULL)
  {
    return pReport->pListAllTests(CU_get_registry());
  }

  /* Silently exit if Report Format does not support feature */
  return 0;
}

/*=================================================================
 *  Static function implementation
 *=================================================================*/
/** Runs the registered tests using the automated interface.
 *  If non-NULL. the specified registry is set as the active
 *  registry for running the tests.  If NULL, then the default
 *  CUnit test registry is used.  The actual test running is
 *  performed by CU_run_all_tests().
 *  @param pRegistry The test registry to run.
 */
static void automated_run_all_tests(CU_pTestRegistry pRegistry)
{
  CU_pTestRegistry pOldRegistry = NULL;

  if (NULL != pRegistry) {
    pOldRegistry = CU_set_registry(pRegistry);
  }

  CU_run_all_tests();

  if (NULL != pRegistry) {
    CU_set_registry(pOldRegistry);
  }
}

/*------------------------------------------------------------------------*/
/** Set tests suites package name
 */
void CU_automated_package_name_set(const char *pName)
{
  memset(_gPackageName, 0, sizeof(_gPackageName));

  /* Is object valid? */
  if (pName) {
    strncpy(_gPackageName, pName, sizeof(_gPackageName) - 1);
    _gPackageName[sizeof(_gPackageName) - 1] = '\0';
  }
}

/*------------------------------------------------------------------------*/
/** Get tests suites package name
 */
const char *CU_automated_package_name_get()
{
 return _gPackageName;
}
/** @} */
