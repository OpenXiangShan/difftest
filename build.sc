/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
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

import os.Path
import mill._
import scalalib._
import publish._

object ivys {
  val scala = "2.13.10"
  val chiselCrossVersions = Map(
    "3.5.6" -> (ivy"edu.berkeley.cs::chisel3:3.5.6", ivy"edu.berkeley.cs:::chisel3-plugin:3.5.6"),
    "3.6.0" -> (ivy"edu.berkeley.cs::chisel3:3.6.0", ivy"edu.berkeley.cs:::chisel3-plugin:3.6.0"),
    "6.0.0-M3" -> (ivy"org.chipsalliance::chisel:6.0.0-M3", ivy"org.chipsalliance:::chisel-plugin:6.0.0-M3"),
  )
}

trait CommonDiffTest extends ScalaModule with SbtModule with Cross.Module[String] {

  override def scalaVersion = ivys.scala

  override def scalacPluginIvyDeps = Agg(ivys.chiselCrossVersions(crossValue)._2)

  override def scalacOptions = Seq("-Ymacro-annotations") ++
    Seq("-feature", "-language:reflectiveCalls")

  override def ivyDeps = Agg(ivys.chiselCrossVersions(crossValue)._1)
}

object difftest extends CommonDiffTest {
  def crossValue: String = "3.5.6"

  override def millSourcePath = os.pwd

  object test extends SbtModuleTests with TestModule.ScalaTest
}
