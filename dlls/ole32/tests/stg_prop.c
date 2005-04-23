/* IPropertyStorage unit tests
 * Copyright 2005 Juan Lang
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>
#define COBJMACROS
#include "objbase.h"
#include "wine/test.h"

#ifdef NONAMELESSUNION
# define U(x)  (x).u
#else
# define U(x)  (x)
#endif

static HRESULT (WINAPI *pFmtIdToPropStgName)(const FMTID *, LPOLESTR);
static HRESULT (WINAPI *pPropStgNameToFmtId)(const LPOLESTR, FMTID *);
static HRESULT (WINAPI *pStgCreatePropSetStg)(IStorage *, DWORD, IPropertySetStorage **);

static void init_function_pointers(void)
{
    HMODULE hmod = GetModuleHandleA("ole32.dll");

    if(hmod)
    {
        pFmtIdToPropStgName = (void*)GetProcAddress(hmod, "FmtIdToPropStgName");
        pPropStgNameToFmtId = (void*)GetProcAddress(hmod, "PropStgNameToFmtId");
        pStgCreatePropSetStg = (void*)GetProcAddress(hmod, "StgCreatePropSetStg");
    }
}
/* FIXME: this creates an ANSI storage, try to find conditions under which
 * Unicode translation fails
 */
static void testProps(void)
{
    static const WCHAR szDot[] = { '.',0 };
    static const WCHAR szPrefix[] = { 's','t','g',0 };
    static const WCHAR propName[] = { 'p','r','o','p',0 };
    static const char val[] = "l33t auth0r";
    WCHAR filename[MAX_PATH];
    HRESULT hr;
    IStorage *storage = NULL;
    IPropertySetStorage *propSetStorage = NULL;
    IPropertyStorage *propertyStorage = NULL;
    PROPSPEC spec;
    PROPVARIANT var;

    if(!GetTempFileNameW(szDot, szPrefix, 0, filename))
        return;

    DeleteFileW(filename);

    hr = StgCreateDocfile(filename,
     STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, &storage);
    ok(SUCCEEDED(hr), "StgCreateDocfile failed: 0x%08lx\n", hr);

    if(!pStgCreatePropSetStg) return;
    hr = pStgCreatePropSetStg(storage, 0, &propSetStorage);
    ok(SUCCEEDED(hr), "StgCreatePropSetStg failed: 0x%08lx\n", hr);

    hr = IPropertySetStorage_Create(propSetStorage,
     &FMTID_SummaryInformation, NULL, PROPSETFLAG_ANSI,
     STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
     &propertyStorage);
    ok(SUCCEEDED(hr), "IPropertySetStorage_Create failed: 0x%08lx\n", hr);

    hr = IPropertyStorage_WriteMultiple(propertyStorage, 0, NULL, NULL, 0);
    ok(SUCCEEDED(hr), "WriteMultiple with 0 args failed: 0x%08lx\n", hr);
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, NULL, NULL, 0);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got 0x%08lx\n", hr);

    /* test setting one that I can't set */
    spec.ulKind = PRSPEC_PROPID;
    U(spec).propid = PID_DICTIONARY;
    PropVariantClear(&var);
    var.vt = VT_I4;
    U(var).lVal = 1;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(hr == STG_E_INVALIDPARAMETER,
     "Expected STG_E_INVALIDPARAMETER, got 0x%08lx\n", hr);

    /* test setting one by name with an invalid propidNameFirst */
    spec.ulKind = PRSPEC_LPWSTR;
    U(spec).lpwstr = (LPOLESTR)propName;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var,
     PID_DICTIONARY);
    ok(hr == STG_E_INVALIDPARAMETER,
     "Expected STG_E_INVALIDPARAMETER, got 0x%08lx\n", hr);

    /* test setting behavior (case-sensitive) */
    spec.ulKind = PRSPEC_PROPID;
    U(spec).propid = PID_BEHAVIOR;
    U(var).lVal = 1;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(hr == STG_E_INVALIDPARAMETER,
     "Expected STG_E_INVALIDPARAMETER, got 0x%08lx\n", hr);

    /* set one by value.. */
    spec.ulKind = PRSPEC_PROPID;
    U(spec).propid = PID_FIRST_USABLE;
    U(var).lVal = 1;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(SUCCEEDED(hr), "WriteMultiple failed: 0x%08lx\n", hr);

    /* set one by name */
    spec.ulKind = PRSPEC_LPWSTR;
    U(spec).lpwstr = (LPOLESTR)propName;
    U(var).lVal = 2;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var,
     PID_FIRST_USABLE);
    ok(SUCCEEDED(hr), "WriteMultiple failed: 0x%08lx\n", hr);

    /* set a string value */
    spec.ulKind = PRSPEC_PROPID;
    U(spec).propid = PIDSI_AUTHOR;
    var.vt = VT_LPSTR;
    U(var).pszVal = (LPSTR)val;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(SUCCEEDED(hr), "WriteMultiple failed: 0x%08lx\n", hr);

    /* check reading */
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 0, NULL, NULL);
    ok(SUCCEEDED(hr), "ReadMultiple with 0 args failed: 0x%08lx\n", hr);
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, NULL, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got 0x%08lx\n", hr);
    /* read by propid */
    spec.ulKind = PRSPEC_PROPID;
    U(spec).propid = PID_FIRST_USABLE;
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(SUCCEEDED(hr), "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_I4 && U(var).lVal == 1,
     "Didn't get expected type or value for property (got type %d, value %ld)\n",
     var.vt, U(var).lVal);
    /* read by name */
    spec.ulKind = PRSPEC_LPWSTR;
    U(spec).lpwstr = (LPOLESTR)propName;
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(SUCCEEDED(hr), "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_I4 && U(var).lVal == 2,
     "Didn't get expected type or value for property (got type %d, value %ld)\n",
     var.vt, U(var).lVal);
    /* read string value */
    spec.ulKind = PRSPEC_PROPID;
    U(spec).propid = PIDSI_AUTHOR;
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(SUCCEEDED(hr), "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_LPSTR && !lstrcmpA(U(var).pszVal, val),
     "Didn't get expected type or value for property (got type %d, value %s)\n",
     var.vt, U(var).pszVal);

    /* check deleting */
    hr = IPropertyStorage_DeleteMultiple(propertyStorage, 0, NULL);
    ok(SUCCEEDED(hr), "DeleteMultiple with 0 args failed: 0x%08lx\n", hr);
    hr = IPropertyStorage_DeleteMultiple(propertyStorage, 1, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got 0x%08lx\n", hr);
    /* contrary to what the docs say, you can't delete the dictionary */
    spec.ulKind = PRSPEC_PROPID;
    U(spec).propid = PID_DICTIONARY;
    hr = IPropertyStorage_DeleteMultiple(propertyStorage, 1, &spec);
    ok(hr == STG_E_INVALIDPARAMETER,
     "Expected STG_E_INVALIDPARAMETER, got 0x%08lx\n", hr);
    /* now delete the first value.. */
    U(spec).propid = PID_FIRST_USABLE;
    hr = IPropertyStorage_DeleteMultiple(propertyStorage, 1, &spec);
    ok(SUCCEEDED(hr), "DeleteMultiple failed: 0x%08lx\n", hr);
    /* and check that it's no longer readable */
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(hr == S_FALSE, "Expected S_FALSE, got 0x%08lx\n", hr);

    hr = IPropertyStorage_Commit(propertyStorage, STGC_DEFAULT);
    ok(SUCCEEDED(hr), "Commit failed: 0x%08lx\n", hr);

    /* check reverting */
    spec.ulKind = PRSPEC_PROPID;
    U(spec).propid = PID_FIRST_USABLE;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(SUCCEEDED(hr), "WriteMultiple failed: 0x%08lx\n", hr);
    hr = IPropertyStorage_Revert(propertyStorage);
    ok(SUCCEEDED(hr), "Revert failed: 0x%08lx\n", hr);
    /* now check that it's still not there */
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(hr == S_FALSE, "Expected S_FALSE, got 0x%08lx\n", hr);
    /* set an integer value again */
    spec.ulKind = PRSPEC_PROPID;
    U(spec).propid = PID_FIRST_USABLE;
    var.vt = VT_I4;
    U(var).lVal = 1;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(SUCCEEDED(hr), "WriteMultiple failed: 0x%08lx\n", hr);
    /* commit it */
    hr = IPropertyStorage_Commit(propertyStorage, STGC_DEFAULT);
    ok(SUCCEEDED(hr), "Commit failed: 0x%08lx\n", hr);
    /* set it to a string value */
    var.vt = VT_LPSTR;
    U(var).pszVal = (LPSTR)val;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(SUCCEEDED(hr), "WriteMultiple failed: 0x%08lx\n", hr);
    /* revert it */
    hr = IPropertyStorage_Revert(propertyStorage);
    ok(SUCCEEDED(hr), "Revert failed: 0x%08lx\n", hr);
    /* Oddly enough, there's no guarantee that a successful revert actually
     * implies the value wasn't saved.  Maybe transactional mode needs to be
     * used for that?
     */

    IPropertyStorage_Release(propertyStorage);
    propertyStorage = NULL;
    IPropertySetStorage_Release(propSetStorage);
    propSetStorage = NULL;
    IStorage_Release(storage);
    storage = NULL;

    /* now open it again */
    hr = StgOpenStorage(filename, NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
     NULL, 0, &storage);
    ok(SUCCEEDED(hr), "StgOpenStorage failed: 0x%08lx\n", hr);

    hr = pStgCreatePropSetStg(storage, 0, &propSetStorage);
    ok(SUCCEEDED(hr), "StgCreatePropSetStg failed: 0x%08lx\n", hr);

    hr = IPropertySetStorage_Open(propSetStorage, &FMTID_SummaryInformation,
     STGM_READWRITE | STGM_SHARE_EXCLUSIVE, &propertyStorage);
    ok(SUCCEEDED(hr), "IPropertySetStorage_Open failed: 0x%08lx\n", hr);

    /* check properties again */
    spec.ulKind = PRSPEC_LPWSTR;
    U(spec).lpwstr = (LPOLESTR)propName;
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(SUCCEEDED(hr), "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_I4 && U(var).lVal == 2,
     "Didn't get expected type or value for property (got type %d, value %ld)\n",
     var.vt, U(var).lVal);
    spec.ulKind = PRSPEC_PROPID;
    U(spec).propid = PIDSI_AUTHOR;
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(SUCCEEDED(hr), "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_LPSTR && !lstrcmpA(U(var).pszVal, val),
     "Didn't get expected type or value for property (got type %d, value %s)\n",
     var.vt, U(var).pszVal);

    IPropertyStorage_Release(propertyStorage);
    IPropertySetStorage_Release(propSetStorage);
    IStorage_Release(storage);

    DeleteFileW(filename);
}

void testCodepage(void)
{
    static const WCHAR szDot[] = { '.',0 };
    static const WCHAR szPrefix[] = { 's','t','g',0 };
    static const WCHAR wval[] = { 'h','i',0 };
    HRESULT hr;
    IStorage *storage = NULL;
    IPropertySetStorage *propSetStorage = NULL;
    IPropertyStorage *propertyStorage = NULL;
    PROPSPEC spec;
    PROPVARIANT var;
    WCHAR fileName[MAX_PATH];

    if(!GetTempFileNameW(szDot, szPrefix, 0, fileName))
        return;

    hr = StgCreateDocfile(fileName,
     STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, &storage);
    ok(SUCCEEDED(hr), "StgCreateDocfile failed: 0x%08lx\n", hr);

    hr = StgCreatePropSetStg(storage, 0, &propSetStorage);
    ok(SUCCEEDED(hr), "StgCreatePropSetStg failed: 0x%08lx\n", hr);

    hr = IPropertySetStorage_Create(propSetStorage,
     &FMTID_SummaryInformation, NULL, PROPSETFLAG_DEFAULT,
     STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
     &propertyStorage);
    ok(SUCCEEDED(hr), "IPropertySetStorage_Create failed: 0x%08lx\n", hr);

    PropVariantInit(&var);
    spec.ulKind = PRSPEC_PROPID;
    U(spec).propid = PID_CODEPAGE;
    /* check code page before it's been explicitly set */
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(SUCCEEDED(hr), "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_I2 && U(var).iVal == 1200,
     "Didn't get expected type or value for property\n");
    /* Set the code page to ascii */
    var.vt = VT_I2;
    U(var).iVal = 1252;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(SUCCEEDED(hr), "WriteMultiple failed: 0x%08lx\n", hr);
    /* check code page */
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(SUCCEEDED(hr), "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_I2 && U(var).iVal == 1252,
     "Didn't get expected type or value for property\n");
    /* Set code page to Unicode */
    U(var).iVal = 1200;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(SUCCEEDED(hr), "WriteMultiple failed: 0x%08lx\n", hr);
    /* check code page */
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(SUCCEEDED(hr), "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_I2 && U(var).iVal == 1200,
     "Didn't get expected type or value for property\n");
    /* Set a string value */
    spec.ulKind = PRSPEC_PROPID;
    U(spec).propid = PID_FIRST_USABLE;
    var.vt = VT_LPSTR;
    U(var).pszVal = "hi";
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(SUCCEEDED(hr), "WriteMultiple failed: 0x%08lx\n", hr);
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(SUCCEEDED(hr), "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_LPSTR && !strcmp(U(var).pszVal, "hi"),
     "Didn't get expected type or value for property\n");
    /* This seemingly non-sensical test is to show that the string is indeed
     * interpreted according to the current system code page, not according to
     * the property set's code page.  (If the latter were true, the whole
     * string would be maintained.  As it is, only the first character is.)
     */
    U(var).pszVal = (LPSTR)wval;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(SUCCEEDED(hr), "WriteMultiple failed: 0x%08lx\n", hr);
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(SUCCEEDED(hr), "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_LPSTR && !strcmp(U(var).pszVal, "h"),
     "Didn't get expected type or value for property\n");
    /* now that a property's been set, you can't change the code page */
    spec.ulKind = PRSPEC_PROPID;
    U(spec).propid = PID_CODEPAGE;
    var.vt = VT_I2;
    U(var).iVal = 1200;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(hr == STG_E_INVALIDPARAMETER,
     "Expected STG_E_INVALIDPARAMETER, got 0x%08lx\n", hr);

    IPropertyStorage_Release(propertyStorage);
    IPropertySetStorage_Release(propSetStorage);
    IStorage_Release(storage);

    DeleteFileW(fileName);

    /* same tests, but with PROPSETFLAG_ANSI */
    hr = StgCreateDocfile(fileName,
     STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, &storage);
    ok(SUCCEEDED(hr), "StgCreateDocfile failed: 0x%08lx\n", hr);

    hr = StgCreatePropSetStg(storage, 0, &propSetStorage);
    ok(SUCCEEDED(hr), "StgCreatePropSetStg failed: 0x%08lx\n", hr);

    hr = IPropertySetStorage_Create(propSetStorage,
     &FMTID_SummaryInformation, NULL, PROPSETFLAG_ANSI,
     STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
     &propertyStorage);
    ok(SUCCEEDED(hr), "IPropertySetStorage_Create failed: 0x%08lx\n", hr);

    /* check code page before it's been explicitly set */
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(SUCCEEDED(hr), "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_I2 && U(var).iVal == 1252,
     "Didn't get expected type or value for property\n");
    /* Set code page to Unicode */
    U(var).iVal = 1200;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(SUCCEEDED(hr), "WriteMultiple failed: 0x%08lx\n", hr);
    /* check code page */
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(SUCCEEDED(hr), "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_I2 && U(var).iVal == 1200,
     "Didn't get expected type or value for property\n");
    /* This test is commented out for documentation.  It fails under Wine,
     * and I expect it would under Windows as well, yet it succeeds.  There's
     * obviously something about string conversion I don't understand.
     */
#if 0
    static const char strVal[] = { 0x81, 0xff, 0x04, 0 };
    /* Set code page to 950 (Traditional Chinese) */
    U(var).iVal = 950;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(SUCCEEDED(hr), "WriteMultiple failed: 0x%08lx\n", hr);
    /* Try writing an invalid string: lead byte 0x81 is unused in Traditional
     * Chinese.
     */
    spec.ulKind = PRSPEC_PROPID;
    U(spec).propid = PID_FIRST_USABLE;
    var.vt = VT_LPSTR;
    U(var).pszVal = (LPSTR)strVal;
    hr = IPropertyStorage_WriteMultiple(propertyStorage, 1, &spec, &var, 0);
    ok(SUCCEEDED(hr), "WriteMultiple failed: 0x%08lx\n", hr);
    /* Check returned string */
    hr = IPropertyStorage_ReadMultiple(propertyStorage, 1, &spec, &var);
    ok(SUCCEEDED(hr), "ReadMultiple failed: 0x%08lx\n", hr);
    ok(var.vt == VT_LPSTR && !strcmp(U(var).pszVal, strVal),
     "Didn't get expected type or value for property\n");
#endif

    IPropertyStorage_Release(propertyStorage);
    IPropertySetStorage_Release(propSetStorage);
    IStorage_Release(storage);

    DeleteFileW(fileName);
}

static void testFmtId(void)
{
    WCHAR szSummaryInfo[] = { 5,'S','u','m','m','a','r','y',
        'I','n','f','o','r','m','a','t','i','o','n',0 };
    WCHAR szDocSummaryInfo[] = { 5,'D','o','c','u','m','e','n','t',
        'S','u','m','m','a','r','y','I','n','f','o','r','m','a','t','i','o','n',
        0 };
    WCHAR szIID_IPropSetStg[] = { 5,'0','j','a','a','a','a','a',
        'a','A','a','a','a','a','a','d','a','A','a','a','a','a','a','a','a','G',
        'c',0 };
    WCHAR name[32];
    FMTID fmtid;
    HRESULT hr;

    if (pFmtIdToPropStgName) {
    hr = pFmtIdToPropStgName(NULL, name);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got 0x%08lx\n", hr);
    hr = pFmtIdToPropStgName(&FMTID_SummaryInformation, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got 0x%08lx\n", hr);
    hr = pFmtIdToPropStgName(&FMTID_SummaryInformation, name);
    ok(SUCCEEDED(hr), "FmtIdToPropStgName failed: 0x%08lx\n", hr);
    ok(!memcmp(name, szSummaryInfo, (lstrlenW(szSummaryInfo) + 1) *
     sizeof(WCHAR)), "Got wrong name for FMTID_SummaryInformation\n");
    hr = pFmtIdToPropStgName(&FMTID_DocSummaryInformation, name);
    ok(SUCCEEDED(hr), "FmtIdToPropStgName failed: 0x%08lx\n", hr);
    ok(!memcmp(name, szDocSummaryInfo, (lstrlenW(szDocSummaryInfo) + 1) *
     sizeof(WCHAR)), "Got wrong name for FMTID_DocSummaryInformation\n");
    hr = pFmtIdToPropStgName(&FMTID_UserDefinedProperties, name);
    ok(SUCCEEDED(hr), "FmtIdToPropStgName failed: 0x%08lx\n", hr);
    ok(!memcmp(name, szDocSummaryInfo, (lstrlenW(szDocSummaryInfo) + 1) *
     sizeof(WCHAR)), "Got wrong name for FMTID_DocSummaryInformation\n");
    hr = pFmtIdToPropStgName(&IID_IPropertySetStorage, name);
    ok(SUCCEEDED(hr), "FmtIdToPropStgName failed: 0x%08lx\n", hr);
    ok(!memcmp(name, szIID_IPropSetStg, (lstrlenW(szIID_IPropSetStg) + 1) *
     sizeof(WCHAR)), "Got wrong name for IID_IPropertySetStorage\n");
    }

    if(pPropStgNameToFmtId) {
    /* test args first */
    hr = pPropStgNameToFmtId(NULL, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got 0x%08lx\n", hr);
    hr = pPropStgNameToFmtId(NULL, &fmtid);
    ok(hr == STG_E_INVALIDNAME, "Expected STG_E_INVALIDNAME, got 0x%08lx\n",
     hr);
    hr = pPropStgNameToFmtId(szDocSummaryInfo, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got 0x%08lx\n", hr);
    /* test the known format IDs */
    hr = pPropStgNameToFmtId(szSummaryInfo, &fmtid);
    ok(SUCCEEDED(hr), "PropStgNameToFmtId failed: 0x%08lx\n", hr);
    ok(!memcmp(&fmtid, &FMTID_SummaryInformation, sizeof(fmtid)),
     "Got unexpected FMTID, expected FMTID_SummaryInformation\n");
    hr = pPropStgNameToFmtId(szDocSummaryInfo, &fmtid);
    ok(SUCCEEDED(hr), "PropStgNameToFmtId failed: 0x%08lx\n", hr);
    ok(!memcmp(&fmtid, &FMTID_DocSummaryInformation, sizeof(fmtid)),
     "Got unexpected FMTID, expected FMTID_DocSummaryInformation\n");
    /* test another GUID */
    hr = pPropStgNameToFmtId(szIID_IPropSetStg, &fmtid);
    ok(SUCCEEDED(hr), "PropStgNameToFmtId failed: 0x%08lx\n", hr);
    ok(!memcmp(&fmtid, &IID_IPropertySetStorage, sizeof(fmtid)),
     "Got unexpected FMTID, expected IID_IPropertySetStorage\n");
    /* now check case matching */
    CharUpperW(szDocSummaryInfo + 1);
    hr = pPropStgNameToFmtId(szDocSummaryInfo, &fmtid);
    ok(SUCCEEDED(hr), "PropStgNameToFmtId failed: 0x%08lx\n", hr);
    ok(!memcmp(&fmtid, &FMTID_DocSummaryInformation, sizeof(fmtid)),
     "Got unexpected FMTID, expected FMTID_DocSummaryInformation\n");
    CharUpperW(szIID_IPropSetStg + 1);
    hr = pPropStgNameToFmtId(szIID_IPropSetStg, &fmtid);
    ok(SUCCEEDED(hr), "PropStgNameToFmtId failed: 0x%08lx\n", hr);
    ok(!memcmp(&fmtid, &IID_IPropertySetStorage, sizeof(fmtid)),
     "Got unexpected FMTID, expected IID_IPropertySetStorage\n");
}
}

START_TEST(stg_prop)
{
    init_function_pointers();
    testProps();
    testCodepage();
    testFmtId();
}
