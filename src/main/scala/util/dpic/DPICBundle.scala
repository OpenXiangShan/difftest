package difftest.util.dpic

import chisel3._

trait DPICRequirements {
  val in: Bundle
  val out: Bundle
  val clock: Bool
  val reset: Bool
}

trait DPICBundle extends Bundle with DPICRequirements{ this: DPICBundle =>
  override def className: String = this.getClass.getSimpleName.replace("$", "")
}