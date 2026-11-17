/***************************************************************************************
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

import mill._
import mill.api.PathRef
import scalalib._
import publish._

sealed trait ChiselConfig {
  val scalaVersion: String = "2.13.18"
  val scalatest: Dep = mvn"org.scalatest::scalatest:3.2.20"
  def chisel: Dep
  def chiselPlugin: Dep
}

object ChiselChipsalliance extends ChiselConfig {
  val chisel = mvn"org.chipsalliance::chisel:7.16.0"
  val chiselPlugin = mvn"org.chipsalliance:::chisel-plugin:7.16.0"
}

object ChiselXiangShan extends ChiselConfig {
  val chisel = mvn"cc.xiangshan::chisel:7.16.0-xiangshan-1"
  val chiselPlugin = mvn"cc.xiangshan:::chisel-plugin:7.16.0-xiangshan-1"
}

trait CommonDiffTest extends ScalaModule with SbtModule {
  def chiselConfig: ChiselConfig

  override def scalaVersion = chiselConfig.scalaVersion

  override def scalacPluginIvyDeps = Agg(chiselConfig.chiselPlugin)

  override def scalacOptions = Seq("-Ymacro-annotations") ++
    Seq("-feature", "-language:reflectiveCalls")

  override def ivyDeps = Agg(chiselConfig.chisel)
}

object design extends DiffTestModule {
  val chiselConfig = ChiselXiangShan
}

object `design-legacy` extends DiffTestModule {
  val chiselConfig = ChiselChipsalliance
}

trait DiffTestModule extends CommonDiffTest {
  override def millSourcePath = os.Path(sys.env("MILL_WORKSPACE_ROOT"))

  override def scalacOptions = super.scalacOptions() ++
    Seq("-Xfatal-warnings", "-deprecation:false", "-unchecked")
}

object difftest extends Difftest {
  val chiselConfig = ChiselXiangShan
}

object `difftest-legacy` extends Difftest {
  val chiselConfig = ChiselChipsalliance
}

trait Difftest extends CommonDiffTest {
  override def millSourcePath = os.Path(sys.env("MILL_WORKSPACE_ROOT"))

  object test extends SbtTests with TestModule.ScalaTest {
    override def ivyDeps = super.ivyDeps() ++ Agg(chiselConfig.scalatest)
  }
}
