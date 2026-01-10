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

object Validate {
  def apply(
    bundles: DecoupledIO[MixedVec[DifftestBundle]],
    config: GatewayConfig,
  ): DecoupledIO[MixedVec[Valid[DifftestBundle]]] = {
    val module = Module(new Validator(chiselTypeOf(bundles.bits).toSeq, config))
    module.in <> bundles
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
  val in = IO(Flipped(Decoupled(MixedVec(bundles))))
  val out = IO(Decoupled(MixedVec(bundles.map(Valid(_)))))
  val globalEnable = WireInit(true.B)
  if (config.hasGlobalEnable) {
    globalEnable := VecInit(in.bits.flatMap(_.bits.needUpdate).toSeq).asUInt.orR
  }
  in.bits.zip(out.bits).foreach { case (i, o) =>
    val valid = i.bits.getValid && globalEnable && in.fire && Option
      .when(i.updateDependency.nonEmpty)(
        VecInit(
          in.bits
            .filter(b => i.updateDependency.contains(b.desiredCppName))
            .map(bundle => {
              // Only if the corresponding bundle is valid, we update this bundle
              bundle.coreid === i.coreid && bundle.bits.getValid
            })
            .toSeq
        ).asUInt.orR
      )
      .getOrElse(true.B)
    o := i.genValidBundle(valid)
  }
  in.ready := out.ready
  out.valid := in.valid && globalEnable
}
