/***************************************************************************************
 * Copyright (c) 2024 Beijing Institute of Open Source Chip (BOSC)
 * Copyright (c) 2020-2024 Institute of Computing Technology, Chinese Academy of Sciences
 *
 * DiffTest is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

package difftest.validate

import chisel3._
import chisel3.util._
import difftest._
import difftest.gateway.GatewayConfig
//import difftest.util.Delayer

object Validate {
  def apply(bundles: MixedVec[DifftestBundle], config: GatewayConfig): MixedVec[Valid[DifftestBundle]] = {
    val module = Module(new Validator(chiselTypeOf(bundles).toSeq, config))
    module.in := bundles
    module.out
  }

  implicit class ValidateHelper(bundle: Valid[DifftestBundle]) {
    def inheritFrom(parent: Valid[DifftestBundle]): Unit = {
      bundle.valid := parent.valid
      bundle.bits.asInstanceOf[DiffTestIsInherited].inheritFrom(parent.bits)
    }
    def supportsSquashBase: Bool = bundle.bits.supportsSquashBase
    def supportsSquash(base: Valid[DifftestBundle]): Bool = bundle.bits.supportsSquash(base.bits)
    def squash(base: Valid[DifftestBundle]): Valid[DifftestBundle] = {
      if (!bundle.bits.bits.hasValid) {
        WireInit(Mux(bundle.valid, bundle, base))
      } else {
        val gen = bundle.bits.squash(base.bits)
        val squashed = WireInit(0.U.asTypeOf(chiselTypeOf(bundle)))
        squashed.bits := gen
        squashed.valid := gen.bits.getValid
        squashed
      }
    }
  }
}

class Validator(bundles: Seq[DifftestBundle], config: GatewayConfig) extends Module {
  val in = IO(Input(MixedVec(bundles)))
  val out = IO(Output(MixedVec(bundles.map(Valid(_)))))
  val globalEnable = WireInit(true.B)
  if (config.hasGlobalEnable) {
    globalEnable := VecInit(in.flatMap(_.bits.needUpdate).toSeq).asUInt.orR
  }
  in.zip(out).foreach { case (i, o) =>
    val updateValid = globalEnable && !reset.asBool &&
      Option.when(i.updateDependency.nonEmpty)(
          VecInit(
            in.filter(b => i.updateDependency.contains(b.desiredCppName))
              .map(bundle => {
                // Only if the corresponding bundle is valid, we update this bundle
                bundle.coreid === i.coreid && bundle.bits.getValid
              })
              .toSeq
          ).asUInt.orR
        )
        .getOrElse(true.B)
    val initValid = Option.when(i.deltaValidLimit.isDefined)(
      // Init all elems at reset ends
      RegNext(reset.asBool) && !reset.asBool
    ).getOrElse(false.B)
    val valid = i.bits.getValid && (updateValid || initValid)
    o := i.genValidBundle(valid)
  }
}
