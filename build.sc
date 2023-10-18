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
  val chisel3 = ivy"edu.berkeley.cs::chisel3:3.5.6"
  val chisel3Plugin = ivy"edu.berkeley.cs:::chisel3-plugin:3.5.6"
  val scalatest = ivy"org.scalatest::scalatest:3.2.2"
}

trait CommonDiffTest extends ScalaModule with SbtModule {
  override def scalaVersion = ivys.scala

  override def scalacPluginIvyDeps = Agg(ivys.chisel3Plugin)

  override def scalacOptions = Seq("-Ymacro-annotations") ++
    Seq("-Xfatal-warnings", "-feature", "-deprecation", "-language:reflectiveCalls")

  override def ivyDeps = Agg(ivys.chisel3)
}

object difftest extends CommonDiffTest {
  override def millSourcePath = os.pwd

  object test extends SbtModuleTests with TestModule.ScalaTest
}
