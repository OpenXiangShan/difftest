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

object ivys {
  val scala = "2.13.14"
  val chisel = (ivy"org.chipsalliance::chisel:7.3.0", ivy"org.chipsalliance:::chisel-plugin:7.3.0")
}

trait CommonDiffTest extends ScalaModule with SbtModule {

  override def scalaVersion = ivys.scala

  override def scalacPluginIvyDeps = Agg(ivys.chisel._2)

  override def scalacOptions = Seq("-Ymacro-annotations") ++
    Seq("-feature", "-language:reflectiveCalls")

  override def ivyDeps = Agg(ivys.chisel._1)
}

object design extends DiffTestModule

trait DiffTestModule extends CommonDiffTest {

  override def millSourcePath = os.Path(sys.env("MILL_WORKSPACE_ROOT"))

  override def scalacOptions = super.scalacOptions() ++
    Seq("-Xfatal-warnings", "-deprecation:false", "-unchecked", "-Xlint")

}

object difftest extends Difftest
trait Difftest extends CommonDiffTest { outer =>

  override def millSourcePath = os.Path(sys.env("MILL_WORKSPACE_ROOT"))

  object test extends SbtTests with TestModule.ScalaTest
}
