// ======================================================================
// \title  FatalHandlerImpl.hpp
// \author Reginald Marr
// \brief  hpp file for FatalHandler component implementation class
//
// ======================================================================

#ifndef Zephyr_FatalHandler_HPP
#define Zephyr_FatalHandler_HPP

#include "Zephyr/Svc/FatalHandler/FatalHandlerComponentAc.hpp"

namespace Zephyr {

class FatalHandler : public FatalHandlerComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Construction, initialization, and destruction
    // ----------------------------------------------------------------------

    //! Construct object FatalHandler
    //!
    FatalHandler(const char* const compName /*!< The component name*/
    );

    //! Destroy object FatalHandler
    //!
    ~FatalHandler();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for user-defined typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for FatalReceive
    //!
    void FatalReceive_handler(const FwIndexType portNum, /*!< The port number*/
                              FwEventIdType Id           /*!< The ID of the FATAL event*/
    );
};

}  // end namespace Svc

#endif
