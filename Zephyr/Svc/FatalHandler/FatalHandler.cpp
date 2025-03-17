// ======================================================================
// \title  FatalHandlerImpl.cpp
// \author Reginald Marr
// \brief  cpp file for FatalHandler component implementation class
//
// ======================================================================
#include <FpConfig.hpp>
#include <Fw/Logger/Logger.hpp>
#include <Zephyr/Svc/FatalHandler/FatalHandler.hpp>
#include "cmsis_gcc.h"
#include "zephyr/sys/__assert.h"

namespace Zephyr {

// ----------------------------------------------------------------------
// Construction, initialization, and destruction
// ----------------------------------------------------------------------

FatalHandler ::FatalHandler(const char* const compName) : FatalHandlerComponentBase(compName) {}

FatalHandler ::~FatalHandler() {}

// ----------------------------------------------------------------------
// Handler implementations for user-defined typed input ports
// ----------------------------------------------------------------------

void FatalHandler::FatalReceive_handler(const NATIVE_INT_TYPE portNum, FwEventIdType Id) {
    // for **nix, delay then exit with error code
    Fw::Logger::log("FATAL %" PRI_FwEventIdType "handled.\n", Id);
    // __BKPT(0);
    assert_print("Assert based on %d\n", Id);
    while (true) {}  // Returning might be bad
}

}  // end namespace Svc
