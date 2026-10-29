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

package difftest.util

import chisel3._
import org.json4s.DefaultFormats
import org.json4s.native.{JsonMethods, Serialization}

import java.nio.charset.StandardCharsets
import java.security.MessageDigest

case class UArchProbeField(name: String, width: Int, count: Int) {
  require(name.matches("[A-Za-z_][A-Za-z0-9_]*"), s"Invalid DiffUArchProbe field name: $name")
  require(width > 0, s"DiffUArchProbe field $name must not be empty")
  require(count > 0, s"DiffUArchProbe field $name must contain data")

  val storageWidth: Int = if (width > 64) 64 else width
  val wordsPerElement: Int = (width + 63) / 64
  val storageCount: Int = count * wordsPerElement
}

case class UArchProbeSchema(bundleName: String, fields: Seq[UArchProbeField]) {
  require(bundleName.matches("[A-Za-z_][A-Za-z0-9_]*"), s"Invalid DiffUArchProbe bundle name: $bundleName")
  require(fields.nonEmpty, "DiffUArchProbe payload must not be empty")
  require(fields.map(_.name).distinct.length == fields.length, "DiffUArchProbe payload contains duplicate field paths")

  val payloadBits: Int = fields.map(field => field.width * field.count).sum
  val storageWidths: Seq[Int] = fields.flatMap(field => Seq.fill(field.storageCount)(field.storageWidth))
  lazy val json: String = Serialization.write(this)(DefaultFormats)
  lazy val layoutHash: String = {
    val digest = MessageDigest.getInstance("SHA-256").digest(json.getBytes(StandardCharsets.UTF_8))
    digest.take(6).map("%02x".format(_)).mkString
  }
  lazy val typeSuffix: String = s"$bundleName${layoutHash.toUpperCase}"
  lazy val instanceSuffix: String = {
    val snakeName = bundleName
      .replaceAll("([a-z0-9])([A-Z])", "$1_$2")
      .toLowerCase
    s"${snakeName}_$layoutHash"
  }
}

object UArchProbeSchema {
  private case class PayloadField(field: UArchProbeField, data: Seq[UInt])

  def fromJson(json: String): UArchProbeSchema =
    JsonMethods.parse(json).extract[UArchProbeSchema](DefaultFormats, manifest[UArchProbeSchema])

  def fromBundle(payload: Bundle): UArchProbeSchema = inspect(payload)._1

  def payloadData(payload: Bundle): (UArchProbeSchema, Seq[Seq[UInt]]) = {
    val (schema, fields) = inspect(payload)
    (schema, fields.map(_.data))
  }

  private def inspect(payload: Bundle): (UArchProbeSchema, Seq[PayloadField]) = {
    val bundleName = sanitizeBundleName(payload.getClass.getSimpleName)
    val fields = flatten(payload, "payload")
    val schema = UArchProbeSchema(bundleName, fields.map(_.field))
    (schema, fields)
  }

  private def flatten(data: Data, name: String): Seq[PayloadField] = data match {
    case vec: Vec[_] if vec.nonEmpty && vec.forall(_.isInstanceOf[Bits]) =>
      val values = vec.map(_.asInstanceOf[Bits].asUInt).toSeq
      val widths = values.map(_.getWidth).distinct
      require(widths.length == 1, s"DiffUArchProbe Vec field $name has inconsistent element widths")
      Seq(PayloadField(UArchProbeField(name, widths.head, values.length), values))
    case vec: Vec[_] =>
      vec.zipWithIndex.flatMap { case (element, index) => flatten(element, s"${name}_$index") }.toSeq
    case bits: Bits =>
      Seq(PayloadField(UArchProbeField(name, bits.getWidth, 1), Seq(bits.asUInt)))
    case record: Record =>
      record.elements.toSeq.reverse.flatMap { case (fieldName, fieldData) =>
        flatten(fieldData, s"${name}_$fieldName")
      }
    case _ =>
      throw new IllegalArgumentException(
        s"DiffUArchProbe does not support field $name of type ${data.getClass.getName}"
      )
  }

  private def sanitizeBundleName(name: String): String = {
    val simpleName = name.split("\\$").filter(_.nonEmpty).lastOption.getOrElse("Bundle")
    val sanitized = simpleName.replaceAll("[^A-Za-z0-9_]", "_")
    if (sanitized.headOption.exists(_.isDigit)) s"Bundle$sanitized" else sanitized
  }
}
