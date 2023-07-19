---
breadcrumbs:
- - /developers
  - For Developers
- - /developers/how-tos
  - How-Tos
- - /developers/how-tos/debugging-on-windows
  - Debugging Chromium on Windows
page_name: example-of-working-with-a-dump
title: Example of working with a dump.
---

Let's say that you are looking at a crash dump, so following the first command
from [this
page](/developers/how-tos/debugging-on-windows/windbg-help#TOC-Common-commands-when-working-with-a-crash)
you do:

```none
0:006> !analyze -v
```

Part of what the command prints is:

```none
FAULTING_IP: 
YCWebCameraSource+14c7e
1c414c7e 8b01            mov     eax,dword ptr [ecx]
EXCEPTION_RECORD:  ffffffff -- (.exr 0xffffffffffffffff)
ExceptionAddress: 1c414c7e (YCWebCameraSource+0x00014c7e)
   ExceptionCode: c0000005 (Access violation)
  ExceptionFlags: 00000000
NumberParameters: 2
   Parameter[0]: 00000000
   Parameter[1]: 00000000
Attempt to read from address 00000000
CONTEXT:  00000000 -- (.cxr 0x0;r)
eax=00000000 ebx=0465c528 ecx=00000000 edx=00000500 esi=0465c498 edi=00000000
eip=77c2e1a4 esp=0465c370 ebp=0465c4f0 iopl=0         nv up ei pl nz ac po nc
cs=0023  ss=002b  ds=002b  es=002b  fs=0053  gs=002b             efl=00200212
PROCESS_NAME:  chrome.exe
ERROR_CODE: (NTSTATUS) 0xc0000005 - The instruction at 0x%08lx referenced memory at 0x%08lx. The memory could not be %s.
EXCEPTION_CODE: (NTSTATUS) 0xc0000005 - The instruction at 0x%08lx referenced memory at 0x%08lx. The memory could not be %s.
EXCEPTION_PARAMETER1:  00000000
EXCEPTION_PARAMETER2:  00000000
READ_ADDRESS:  00000000 
FOLLOWUP_IP: 
YCWebCameraSource+14c7e
1c414c7e 8b01            mov     eax,dword ptr [ecx]
BUGCHECK_STR:  APPLICATION_FAULT_NULL_POINTER_READ_INVALID_POINTER_READ_BEFORE_CALL
STACK_TEXT:  
0465cab8 00000000 0465cbc4 096052c0 1c414fe1 YCWebCameraSource+0x14c7e
```

which basically says that there is an attempt to read from NULL:

YCWebCameraSource+14c7e

1c414c7e 8b01 mov eax,dword ptr \[ecx\]

and ECX is 0.

The problematic part is the last one: the stack is a single line of text, inside
a DLL that you know nothing about. Moving on to the next command,

```none
0:006> .ecxr
eax=0bd00048 ebx=000000f0 ecx=00000000 edx=00000500 esi=0465cbc4 edi=00000140
eip=1c414c7e esp=0465cabc ebp=000000f0 iopl=0         nv up ei pl nz na po nc
cs=0023  ss=002b  ds=002b  es=002b  fs=0053  gs=002b             efl=00210202
YCWebCameraSource+0x14c7e:
1c414c7e 8b01            mov     eax,dword ptr [ecx]  ds:002b:00000000=????????
```

confirms what we already know as the debugger loads the context when the
exception was received. The stack window shows only one line of text.

```none
0:006> k
  *** Stack trace for last set context - .thread/.cxr resets it
ChildEBP RetAddr  
WARNING: Stack unwind information not available. Following frames may be wrong.
0465cab8 00000000 YCWebCameraSource+0x14c7e
```

Looking at the registers show the reason: EBP is 0xF0, which is not a pointer to
the stack. ESP looks fine, so let's see what's there using the next command from
the list:

```none
0:006> dds esp
0465cabc  00000000
0465cac0  0465cbc4
0465cac4  096052c0
0465cac8  1c414fe1 YCWebCameraSource+0x14fe1
0465cacc  0b7b0000
0465cad0  00000140
0465cad4  000000f0
0465cad8  00000500
0465cadc  09710000
0465cae0  00000140
0465cae4  000000f0
0465cae8  000003c0
0465caec  00000000
0465caf0  00000000
0465caf4  00000140
0465caf8  1c40c24e YCWebCameraSource+0xc24e
0465cafc  0b7b0000
0465cb00  00000140
0465cb04  000000f0
0465cb08  00000000
0465cb0c  00000500
0465cb10  09710000
0465cb14  00000140
0465cb18  000000f0
0465cb1c  00000000
0465cb20  000003c0
0465cb24  0465e074
0465cb28  09158124
0465cb2c  091561c8
0465cb30  00000000
0465cb34  09605848
0465cb38  0b8007c8
```

A few pointers to code close to the address that crashed, which is good because
it means the control flow before the crash is reasonable. But clearly that code
is not using EBP to maintain stack frames (more on that later). So see what else
is on the stack:

```none
0:006> dds
0465cb3c  0b800960
0465cb40  000003c0
0465cb44  091561c8
0465cb48  00022009
0465cb4c  80004005
0465cb50  1c4238f8 YCWebCameraSource+0x238f8
0465cb54  09605848
0465cb58  00000000
0465cb5c  000000b2
0465cb60  096052c0
0465cb64  00000000
0465cb68  00000000
0465cb6c  00000140
0465cb70  000000f0
0465cb74  00000140
0465cb78  000000f0
0465cb7c  00000500
0465cb80  00022009
0465cb84  0b7b0000
0465cb88  00000001
0465cb8c  1c4238f8 YCWebCameraSource+0x238f8
0465cb90  096046c8
0465cb94  00000000
0465cb98  0465cc28
0465cb9c  00000140
0465cba0  000000f0
0465cba4  000003c0
0465cba8  00021808
0465cbac  09710000
0465cbb0  00000002
0465cbb4  557cf400
0465cbb8  11d31a04
```

Note that issuing the command without an argument just continues where the last
invocation left of. Furthermore, just hitting enter repeats the last command so
after this point is a matter of keep hitting enter. Which is good because in
this case interesting things happen after a long time:

```none
0:006> 
0465f22c  f15f2bff
0465f230  0465f27c
0465f234  772b9b03 msvcrt!free+0x65
0465f238  01700000
0465f23c  00000000
0465f240  772b9b10 msvcrt!free+0x84
0465f244  f1a4c16f
0465f248  00000000
0465f24c  0170b3f8
0465f250  00000000
0465f254  00000000
0465f258  0465f238
0465f25c  ffffffff
0465f260  00000000
0465f264  0465f244
0465f268  82eaa80b
0465f26c  0465f98c
0465f270  772dc265 msvcrt!_except_handler4
0465f274  82eaa80b
0465f278  fffffffe
0465f27c  772b9b10 msvcrt!free+0x84
0465f280  732343a5 devenum!ATL::CComObject<CCreateSwEnum>::`scalar deleting destructor'+0x3d
0465f284  0170b3f8
0465f288  73234566 devenum!ATL::CComObject<CCreateSwEnum>::Release+0x23
0465f28c  03290d90
0465f290  031f0b50
0465f294  0465f4bc
0465f298  6fb647db chrome_6faf0000!media::GetDeviceNamesDirectShow+0x3ab
0465f29c  6fb647e5 chrome_6faf0000!media::GetDeviceNamesDirectShow+0x3b5
0465f2a0  0465f2b8
0465f2a4  6faf22b5 chrome_6faf0000!tcmalloc::FL_Push+0x71
0465f2a8  01192308
```

After a while of seeing symbols from various DLLs go through the stack we start
to see symbols from Chrome. The last part looks relatively good. Note that at
address 0465f298 there is a pointer to Chrome code, and more importantly, 4
bytes before there is a pointer to another place in the stack, some bytes after
the current position: 0465f294 0465f4bc. This is the typical pattern of a call
to a function that use EBP to track stack frames. Time to use the next command
from the list:

```none
0:006> k = 0465f294  0465f294  0465f294  
ChildEBP RetAddr  
WARNING: Frame IP not in any known module. Following frames may be wrong.
0465f294 6fb647db 0x465f294
0465f4bc 07554180 chrome_6faf0000!media::GetDeviceNamesDirectShow+0x3ab
0465f4cc 6fb643d2 0x7554180
0465f4e8 6fb64386 chrome_6faf0000!media::VideoCaptureDeviceFactory::EnumerateDeviceNames+0x45
0465f4f4 6fb55ece chrome_6faf0000!base::internal::Invoker<>
0465f5d4 6fb5552d chrome_6faf0000!base::MessageLoop::RunTask+0x50a
0465f718 6fb62bad chrome_6faf0000!base::MessageLoop::DoWork+0x359
0465f744 6fb5503a chrome_6faf0000!base::MessagePumpDefault::Run+0xc7
0465f768 6fb54f2d chrome_6faf0000!base::MessageLoop::RunHandler+0x6e
0465f794 6fb54ec4 chrome_6faf0000!base::MessageLoop::Run+0x65
0465f79c 6fb5291e chrome_6faf0000!base::Thread::Run+0xb
0465f928 6fb52509 chrome_6faf0000!base::Thread::ThreadMain+0x26e
0465f94c 778a850d chrome_6faf0000!base::`anonymous namespace'::ThreadFunc+0xcb
0465f958 77c5bf39 kernel32!BaseThreadInitThunk+0xe
0465f99c 77c5bf0c ntdll!__RtlUserThreadStart+0x72
0465f9b4 00000000 ntdll!_RtlUserThreadStart+0x1b
```

Strictly speaking the format that I just used is not correct, but the debugger
does a decent job figuring out what I want just complaining about the first
frame. The good part is that now we have a stack that looks reasonable. Time to
go up and try to get something better.

The pattern that we just saw, a pointer a few bytes ahead followed by a symbol
should repeat itself... as in the pointer should point to another pointer
followed by a symbol. Se if we search up on the debugger output for 0465f294 we
should get to the previous frame. If there is no match, it means that either we
reached a function call that doesn't use EBP, or we are following stale data
from the stack (traces of something that happened before, and has not been
overwritten yet, but it is not the current call sequence).

And that's exactly what happens in this case. But going up and following stack
manually a few times provides a better stack. Remember that the goal is not to
get a perfect stack trace but to get enough information to do something about
it. We already know that get the actual stack will be impossible because close
to the crash point there's no EBP and no symbols. But the type of symbols that
you see going through while executing dds gives you a rough idea of how the flow
ends up at the code that crashes.

```none
0:006> k = 0465e0ac  0465e0ac  0465e0ac  
ChildEBP RetAddr  
WARNING: Frame IP not in any known module. Following frames may be wrong.
0465e0ac 76ec7543 0x465e0ac
0465e11c 76ec4fdf combase!CServerContextActivator::CreateInstance+0x18b
0465e15c 76ec7610 combase!ActivationPropertiesIn::DelegateCreateInstance+0x5c
0465e1b0 76ec7334 combase!CApartmentActivator::CreateInstance+0x75
0465e1d4 76ec6d52 combase!CProcessActivator::CCICallback+0x3b
0465e1f4 76ec72cf combase!CProcessActivator::AttemptActivation+0x2c
0465e22c 76ec73a9 combase!CProcessActivator::ActivateByContext+0x97
0465e25c 76ec4fdf combase!CProcessActivator::CreateInstance+0x5d
0465e29c 76ec50d7 combase!ActivationPropertiesIn::DelegateCreateInstance+0x5c
0465e4fc 76ec4fdf combase!CClientContextActivator::CreateInstance+0xdd
0465e53c 76ec5ba6 combase!ActivationPropertiesIn::DelegateCreateInstance+0x5c
0465ede8 76ebc9c2 combase!ICoCreateInstanceEx+0xfb6
(Inline) -------- combase!CComActivator::DoCreateInstance+0x11a
(Inline) -------- combase!CoCreateInstanceEx+0x14e
0465ee3c 732352c3 combase!CoCreateInstance+0x169
0465eef0 70cedfbd devenum!CDeviceMoniker::BindToObject+0x1ac
0465f0e0 70ce59df chrome_6faf0000!media::VideoCaptureDeviceWin::GetDeviceFilter+0x234
0465f3ac 70ce56a2 chrome_6faf0000!media::GetDeviceSupportedFormatsDirectShow+0x335
0465f3c0 6fb64995 chrome_6faf0000!media::VideoCaptureDeviceFactoryWin::GetDeviceSupportedFormats+0x20
0465f44c 6fb648c1 chrome_6faf0000!content::VideoCaptureManager::ConsolidateDevicesInfoOnDeviceThread+0xc1
0465f46c 6fb64883 chrome_6faf0000!base::internal::RunnableAdapter<>::Run+0x2e
0465f488 6fb64848 chrome_6faf0000!base::internal::InvokeHelper<>::MakeItSo+0x25
0465f4b8 6fb64800 chrome_6faf0000!base::internal::Invoker<<media::VideoCaptureDevice::N+0x33
0465f4cc 6fb643d2 chrome_6faf0000!base::Callback<>::Run+0x17
0465f4e8 6fb64386 chrome_6faf0000!media::VideoCaptureDeviceFactory::EnumerateDeviceNames+0x45
0465f4f4 6fb55ece chrome_6faf0000!base::internal::Invoker<2,base::internal::BindState<>::Run+0x10
0465f5d4 6fb5552d chrome_6faf0000!base::MessageLoop::RunTask+0x50a
0465f718 6fb62bad chrome_6faf0000!base::MessageLoop::DoWork+0x359
0465f744 6fb5503a chrome_6faf0000!base::MessagePumpDefault::Run+0xc7
0465f768 6fb54f2d chrome_6faf0000!base::MessageLoop::RunHandler+0x6e
0465f794 6fb54ec4 chrome_6faf0000!base::MessageLoop::Run+0x65
0465f79c 6fb5291e chrome_6faf0000!base::Thread::Run+0xb
0465f928 6fb52509 chrome_6faf0000!base::Thread::ThreadMain+0x26e
0465f94c 778a850d chrome_6faf0000!base::`anonymous namespace'::ThreadFunc+0xcb
0465f958 77c5bf39 kernel32!BaseThreadInitThunk+0xe
0465f99c 77c5bf0c ntdll!__RtlUserThreadStart+0x72
0465f9b4 00000000 ntdll!_RtlUserThreadStart+0x1b
```

Note how this stack is slightly different right where the previous stack
started, but it flows nicely into a sequence of com calls. There's usually no
point in going too deep into code we don't control, so that is all we need in
this case.
