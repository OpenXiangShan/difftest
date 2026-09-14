/***************************************************************************************
 * Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
 * Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
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

package difftest

import chisel3._
import difftest.util.BundleProfile
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatest.matchers.should.Matchers

class DiffUArchProbeSpec extends AnyFlatSpec with Matchers {
  private class TestPayload extends Bundle {
    val flag = Bool()
    val wide = UInt(65.W)
    val lanes = Vec(2, UInt(5.W))
    val nested = new Bundle {
      val count = UInt(12.W)
      val ready = Bool()
    }
  }

  private class SplitBytePayload extends Bundle {
    val low = UInt(4.W)
    val high = UInt(4.W)
  }

  private class WholeBytePayload extends Bundle {
    val value = UInt(8.W)
  }

  "DiffUArchProbe" should "preserve Bundle fields and survive profile reconstruction" in {
    val probe = new DiffUArchProbe(new TestPayload)
    probe.payloadBits shouldBe 89
    probe.data.length shouldBe 7
    (probe.probeSchema.layoutHash should fullyMatch).regex("[0-9a-f]{12}")
    probe.dataElements.map(_._1) shouldBe Seq(
      "valid", "uarchId", "cycleCnt", "payload_flag", "payload_wide", "payload_lanes", "payload_nested_count",
      "payload_nested_ready",
    )
    probe.dataElements.find(_._1 == "payload_wide").get._3.length shouldBe 2
    probe.dataElements.find(_._1 == "payload_lanes").get._3.length shouldBe 2
    probe.physicalElementsInSeqUInt.map(_._1) should contain("data")
    probe.physicalElementsInSeqUInt.map(_._1) should not contain "payload_flag"

    val cpp = probe.toCppDeclaration(packed = true, aligned = false)
    cpp should include("uint8_t  payload_flag;")
    cpp should include("uint64_t payload_wide[2];")
    cpp should include("uint8_t  payload_lanes[2];")
    cpp should include("uint16_t payload_nested_count;")
    cpp should include("uint8_t  payload_nested_ready;")

    val restored = BundleProfile.fromBundle(probe, delay = 0).toBundle
    restored shouldBe a[DiffUArchProbe[_]]
    val restoredProbe = restored.asInstanceOf[DiffUArchProbe[_]]
    restoredProbe.schema shouldBe probe.schema
    restoredProbe.desiredModuleName shouldBe probe.desiredModuleName
    restoredProbe.dataElements.map(_._1) shouldBe probe.dataElements.map(_._1)
  }

  it should "distinguish Bundle layouts with the same total width" in {
    val split = new DiffUArchProbe(new SplitBytePayload)
    val whole = new DiffUArchProbe(new WholeBytePayload)

    split.payloadBits shouldBe whole.payloadBits
    split.desiredModuleName should not be whole.desiredModuleName
    split.desiredCppName should not be whole.desiredCppName
  }

  "BundleProfile" should "continue to reconstruct existing interfaces" in {
    BundleProfile.fromBundle(new DiffArchEvent, delay = 0).toBundle shouldBe a[DiffArchEvent]
    BundleProfile.fromBundle(new DiffArchIntDelayedUpdate, delay = 0).toBundle shouldBe a[DiffArchIntDelayedUpdate]

    val commit = BundleProfile.fromBundle(new DiffInstrCommit(48), delay = 1).toBundle
    commit shouldBe a[DiffInstrCommit]
    commit.asInstanceOf[DiffInstrCommit].numPhyRegs shouldBe 48
  }
}
