/*
 * Copyright 2021 Rémi Bernon for CodeWeavers
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#define COBJMACROS
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winstring.h"

#include "initguid.h"
#include "roapi.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Gaming_Input
#include "windows.gaming.input.h"

#include "wine/test.h"

static HRESULT (WINAPI *pRoActivateInstance)(HSTRING, IInspectable **);
static HRESULT (WINAPI *pRoGetActivationFactory)(HSTRING, REFIID, void **);
static HRESULT (WINAPI *pRoInitialize)(RO_INIT_TYPE);
static void    (WINAPI *pRoUninitialize)(void);
static HRESULT (WINAPI *pWindowsCreateString)(LPCWSTR, UINT32, HSTRING *);
static HRESULT (WINAPI *pWindowsDeleteString)(HSTRING);

struct gamepad_event_handler
{
    IEventHandler_Gamepad IEventHandler_Gamepad_iface;
    LONG ref;
};

static inline struct gamepad_event_handler *impl_from_IEventHandler_Gamepad(IEventHandler_Gamepad *iface)
{
    return CONTAINING_RECORD(iface, struct gamepad_event_handler, IEventHandler_Gamepad_iface);
}

static HRESULT STDMETHODCALLTYPE gamepad_event_handler_QueryInterface(
        IEventHandler_Gamepad *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IEventHandler_Gamepad))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    trace("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE gamepad_event_handler_AddRef(
        IEventHandler_Gamepad *iface)
{
    struct gamepad_event_handler *impl = impl_from_IEventHandler_Gamepad(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE gamepad_event_handler_Release(
        IEventHandler_Gamepad *iface)
{
    struct gamepad_event_handler *impl = impl_from_IEventHandler_Gamepad(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE gamepad_event_handler_Invoke(
        IEventHandler_Gamepad *iface, IInspectable *sender, IGamepad *args)
{
    trace("iface %p, sender %p, args %p\n", iface, sender, args);
    return S_OK;
}

static const IEventHandler_GamepadVtbl gamepad_event_handler_vtbl =
{
    gamepad_event_handler_QueryInterface,
    gamepad_event_handler_AddRef,
    gamepad_event_handler_Release,
    /*** IEventHandler<ABI::Windows::Gaming::Input::Gamepad* > methods ***/
    gamepad_event_handler_Invoke,
};

static void test_Gamepad(void)
{
    static const WCHAR *gamepad_name = L"Windows.Gaming.Input.Gamepad";

    struct gamepad_event_handler gamepad_event_handler;
    EventRegistrationToken token;
    IVectorView_Gamepad *gamepads = NULL;
    IActivationFactory *factory = NULL;
    IGamepadStatics *gamepad_statics = NULL;
    IInspectable *inspectable = NULL, *tmp_inspectable = NULL;
    IAgileObject *agile_object = NULL, *tmp_agile_object = NULL;
    HSTRING str;
    HRESULT hr;
    ULONG size;

    gamepad_event_handler.IEventHandler_Gamepad_iface.lpVtbl = &gamepad_event_handler_vtbl;

    hr = pRoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#x\n", hr);

    hr = pWindowsCreateString(gamepad_name, wcslen(gamepad_name), &str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#x\n", hr);

    hr = pRoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "RoGetActivationFactory failed, hr %#x\n", hr);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w(gamepad_name));
        return;
    }

    hr = IActivationFactory_QueryInterface(factory, &IID_IInspectable, (void **)&inspectable);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IInspectable failed, hr %#x\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IAgileObject, (void **)&agile_object);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IAgileObject failed, hr %#x\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IGamepadStatics, (void **)&gamepad_statics);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IGamepadStatics failed, hr %#x\n", hr);

    hr = IGamepadStatics_QueryInterface(gamepad_statics, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "IGamepadStatics_QueryInterface IID_IInspectable failed, hr %#x\n", hr);
    ok(tmp_inspectable == inspectable, "IGamepadStatics_QueryInterface IID_IInspectable returned %p, expected %p\n", tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);

    hr = IGamepadStatics_QueryInterface(gamepad_statics, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == S_OK, "IGamepadStatics_QueryInterface IID_IAgileObject failed, hr %#x\n", hr);
    ok(tmp_agile_object == agile_object, "IGamepadStatics_QueryInterface IID_IAgileObject returned %p, expected %p\n", tmp_agile_object, agile_object);
    IAgileObject_Release(tmp_agile_object);

    hr = IGamepadStatics_get_Gamepads(gamepad_statics, &gamepads);
    todo_wine ok(hr == S_OK, "IGamepadStatics_get_Gamepads failed, hr %#x\n", hr);
    if (FAILED(hr)) goto done;

    hr = IVectorView_Gamepad_QueryInterface(gamepads, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "IVectorView_Gamepad_QueryInterface failed, hr %#x\n", hr);
    ok(tmp_inspectable != inspectable, "IVectorView_Gamepad_QueryInterface returned %p, expected %p\n", tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);

    hr = IVectorView_Gamepad_QueryInterface(gamepads, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == S_OK, "IVectorView_Gamepad_QueryInterface failed, hr %#x\n", hr);
    ok(tmp_agile_object != agile_object, "IVectorView_Gamepad_QueryInterface IID_IAgileObject returned agile_object\n");
    IAgileObject_Release(tmp_agile_object);

    size = 0xdeadbeef;
    hr = IVectorView_Gamepad_get_Size(gamepads, &size);
    ok(hr == S_OK, "IVectorView_Gamepad_get_Size failed, hr %#x\n", hr);
    todo_wine ok(size != 0xdeadbeef, "IVectorView_Gamepad_get_Size returned %u\n", size);

    IVectorView_Gamepad_Release(gamepads);

done:
    token.value = 0xdeadbeef;
    hr = IGamepadStatics_add_GamepadAdded(gamepad_statics, &gamepad_event_handler.IEventHandler_Gamepad_iface, &token);
    todo_wine ok(hr == S_OK, "IGamepadStatics_add_GamepadAdded failed, hr %#x\n", hr);
    todo_wine ok(token.value != 0xdeadbeef, "IGamepadStatics_add_GamepadAdded returned token %#I64x\n", token.value);

    hr = IGamepadStatics_remove_GamepadAdded(gamepad_statics, token);
    todo_wine ok(hr == S_OK, "IGamepadStatics_add_GamepadAdded failed, hr %#x\n", hr);

    token.value = 0xdeadbeef;
    IGamepadStatics_add_GamepadRemoved(gamepad_statics, &gamepad_event_handler.IEventHandler_Gamepad_iface, &token);
    todo_wine ok(hr == S_OK, "IGamepadStatics_add_GamepadRemoved failed, hr %#x\n", hr);
    todo_wine ok(token.value != 0xdeadbeef, "IGamepadStatics_add_GamepadRemoved returned token %#I64x\n", token.value);

    hr = IGamepadStatics_remove_GamepadRemoved(gamepad_statics, token);
    todo_wine ok(hr == S_OK, "IGamepadStatics_add_GamepadAdded failed, hr %#x\n", hr);

    IGamepadStatics_Release(gamepad_statics);

    IAgileObject_Release(agile_object);
    IInspectable_Release(inspectable);
    IActivationFactory_Release(factory);

    pWindowsDeleteString(str);

    pRoUninitialize();
}

START_TEST(input)
{
    HMODULE combase;

    if (!(combase = LoadLibraryW(L"combase.dll")))
    {
        win_skip("Failed to load combase.dll, skipping tests\n");
        return;
    }

#define LOAD_FUNCPTR(x) \
    if (!(p##x = (void*)GetProcAddress(combase, #x))) \
    { \
        win_skip("Failed to find %s in combase.dll, skipping tests.\n", #x); \
        return; \
    }

    LOAD_FUNCPTR(RoActivateInstance);
    LOAD_FUNCPTR(RoGetActivationFactory);
    LOAD_FUNCPTR(RoInitialize);
    LOAD_FUNCPTR(RoUninitialize);
    LOAD_FUNCPTR(WindowsCreateString);
    LOAD_FUNCPTR(WindowsDeleteString);
#undef LOAD_FUNCPTR

    test_Gamepad();
}
