/***************************************************************************************
# Copyright (c) 2020-2021 Institute of Computing Technology, Chinese Academy of Sciences
# Copyright (c) 2020-2021 Peng Cheng Laboratory
#
# XiangShan is licensed under Mulan PSL v2.
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

trait CommonModule extends ScalaModule {
  override def scalaVersion = "2.12.10"

  override def scalacOptions = Seq("-Xsource:2.11")

  private val macroParadise = ivy"org.scalamacros:::paradise:2.1.0"

  override def compileIvyDeps = Agg(macroParadise)

  override def scalacPluginIvyDeps = Agg(macroParadise)
}

val chisel = Agg(
  ivy"edu.berkeley.cs::chisel3:3.4.3"
)

object difftest extends SbtModule with CommonModule {
  override def millSourcePath = millOuterCtx.millSourcePath
  override def ivyDeps = super.ivyDeps() ++ chisel
}