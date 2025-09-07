// This file addresses compatibility issues for Chisel.

package chisel3 {

  import chisel3.internal._

  object compatibility {
    // Return the internal implicit Clock
    def currentClock: Option[Clock] = Builder.currentClock

    // Return the internal implicit Reset
    def currentReset: Option[Reset] = Builder.currentReset
  }

}
