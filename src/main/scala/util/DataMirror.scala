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

package difftest.util

import chisel3._
import chisel3.experimental.SourceInfo

import scala.reflect.runtime.currentMirror
import scala.reflect.runtime.universe._

private[difftest] object DataMirror {
  private def loadMethodOfObject(methodName: String, objectName: String): Option[MethodMirror] = {
    val moduleSymb = currentMirror.staticModule(objectName)
    val methodSymb = moduleSymb.info.decls.find(m => m.isMethod && m.name.toString == methodName).map(_.asMethod)
    val obj = currentMirror.reflectModule(moduleSymb).instance
    methodSymb.map(s => currentMirror.reflect(obj).reflectMethod(s))
  }

  implicit class DataMirrorLoader[T <: Data](data: T) {
    def isVisible: Boolean = {
      val method = loadMethodOfObject("isVisible", "chisel3.reflect.DataMirror")
      method.exists(_.apply(data).asInstanceOf[Boolean])
    }

    def tapAndRead(implicit si: SourceInfo): T = {
      require(
        !chisel3.BuildInfo.version.startsWith("3"),
        "BoringUtils.tapAndRead does not support Chisel 3, use BoringUtils.addSource/addSink in replace.",
      )
      val method = loadMethodOfObject("tapAndRead", "chisel3.util.experimental.BoringUtils")
      val argument: Seq[Any] = Seq(data, si)
      method.get.apply(argument: _*).asInstanceOf[T]
    }
  }
}
