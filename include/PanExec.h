//
// _PanExec_h_
//
// Copyright (C) 2017-2023 Tactical Computing Laboratories, LLC
// All Rights Reserved
// contact@tactcomplabs.com
//
// See LICENSE in the top level directory for licensing details
//

#ifndef _SST_PANEXEC_H_
#define _SST_PANEXEC_H_

#include <vector>
#include <tuple>
#include <cstdint>

#ifndef _PANEXEC_MAX_ENTRY_
#define _PANEXEC_MAX_ENTRY_ 64
#endif

namespace SST {
  namespace RevCPU {

    class PanExec{
    public:
      enum PanStatus : uint8_t {
        QExec   = 0,                  ///< PanStatus: entry is executing
        QValid  = 1,                  ///< PanStatus: valid entry, ready to execute
        QNull   = 13,                 ///< PanStatus: null entry
        QError  = 0xFF,               ///< PanStatus: error
      };

      /// PanExec: add an execution queue entry
      bool AddEntry(uint64_t Addr, unsigned *Idx);

      /// PanExec: remove an execution queue entry
      bool RemoveEntry(unsigned Idx);

      /// PanExec: status of the entry
      PanStatus StatusEntry(unsigned Idx);

      /// PanExec: get execution entry
      PanStatus GetNextEntry(uint64_t *Addr, unsigned *Idx);

    private:
      // private data members
      unsigned CurEntry = 0;
      std::vector<std::tuple<unsigned, PanStatus, uint64_t>> ExecQueue;    ///< PanExec: execution queue

      // private functions
      unsigned GetNewEntry();

    };  // class PanExec
  } // namespace RevCPU
} // namespace SST


#endif

// EOF
