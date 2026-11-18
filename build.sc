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
  val scala = "2.13.18"
  val chiselCrossVersions = Map(
    "xiangshan" -> (
      mvn"cc.xiangshan::chisel:7.16.0-xiangshan-1",
      mvn"cc.xiangshan:::chisel-plugin:7.16.0-xiangshan-1"
    ),
    "upstream" -> (
      mvn"org.chipsalliance::chisel:7.16.0",
      mvn"org.chipsalliance:::chisel-plugin:7.16.0"
    ),
  )
  val scalatest = mvn"org.scalatest::scalatest:3.2.20"
}

trait CommonDiffTest extends ScalaModule with SbtModule with Cross.Module[String] {
  override def scalaVersion = ivys.scala

  override def scalacPluginIvyDeps = Agg(ivys.chiselCrossVersions(crossValue)._2)

  override def scalacOptions = Seq("-Ymacro-annotations") ++
    Seq("-feature", "-language:reflectiveCalls")

  override def ivyDeps = Agg(ivys.chiselCrossVersions(crossValue)._1)
}

object design extends mill.define.Cross[DiffTestModule](ivys.chiselCrossVersions.keys.toSeq)

trait DiffTestModule extends CommonDiffTest {
  override def millSourcePath = os.Path(sys.env("MILL_WORKSPACE_ROOT"))

  override def scalacOptions = super.scalacOptions() ++
    Seq("-Xfatal-warnings", "-deprecation:false", "-unchecked")
}

object difftest extends mill.define.Cross[Difftest](ivys.chiselCrossVersions.keys.toSeq)

trait Difftest extends CommonDiffTest {
  override def millSourcePath = os.Path(sys.env("MILL_WORKSPACE_ROOT"))

  object test extends SbtTests with TestModule.ScalaTest {
    override def ivyDeps = super.ivyDeps() ++ Agg(ivys.scalatest)
  }
}
