#pragma once
#define _CRTDBG_MAP_ALLOC	1

#include "eck\PchInclude.h"
#include "eck\CTrivialBuffer.h"
#include "eck\CScopeGuard.h"
#include "eck\FileHelper.h"
#include "eck\RefPtr.h"
#include "eck\CEvent.h"
#include "eck\MediaTagMpeg.h"
#include "eck\CSelectionRange.h"

#include <mmreg.h>
#include <MSAcm.h>

#include <thread>

using eck::PCVOID;
using eck::PCBYTE;
using eck::SafeRelease;
using eck::DpiScale;
using eck::W32ERR;
using eck::RefPtr;