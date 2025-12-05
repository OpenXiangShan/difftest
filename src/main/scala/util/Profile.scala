/***************************************************************************************
 * Copyright (c) 2024 Institute of Computing Technology, Chinese Academy of Sciences
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

import difftest.{DifftestBundle, DifftestModule}
import difftest.common.FileControl
import org.json4s.DefaultFormats
import org.json4s.native._

import java.nio.file.{Files, Paths}

// Note: json4s seems to load integers with BigInt. We may need to convert them into int if necessary

case class BundleProfile(
  className: String,
  classArgs: Map[String, Any],
  delay: Int,
) {
  def toBundle: DifftestBundle = {
    val constructor = Class.forName(className).getConstructors()(0)
    val args = constructor.getParameters.map { param =>
      (classArgs(param.getName), param.getType.getName) match {
        case (arg: BigInt, "int") => arg.toInt
        case (arg, _)             => arg
      }
    }
    constructor.newInstance(args: _*).asInstanceOf[DifftestBundle]
  }
}

case class DifftestProfile(
  cpu: String,
  numCores: Int,
  cmdConfigs: Seq[String],
  bundles: Seq[BundleProfile],
) {
  def toJsonString: String = Serialization.writePretty(this)(DefaultFormats)
}

object DifftestProfile {
  def fromBundles(cpu: String, cmdConfigs: Seq[String], bundles: Seq[(DifftestBundle, Int)]): DifftestProfile = {
    val numCores = bundles.count(_._1.isUniqueIdentifier)
    // Note: numCores may be 0 when difftest interfaces are not connected
    require(numCores == 0 || bundles.length % numCores == 0, "cannot create the profile if cores are not symmetric")
    val nBundles = if (numCores > 0) bundles.length / numCores else 0
    val bundleProfiles = bundles.take(nBundles).map { case (b, d) => BundleProfile.fromBundle(b, d) }
    DifftestProfile(cpu, numCores, cmdConfigs, bundleProfiles)
  }

  def fromJson(filename: String): DifftestProfile = {
    val profileStr = new String(Files.readAllBytes(Paths.get(filename)))
    val profiles = JsonMethods.parse(profileStr).extract(DefaultFormats, manifest[Map[String, Any]])
    val cpu = profiles("cpu").asInstanceOf[String]
    val numCores = profiles("numCores").asInstanceOf[BigInt].toInt
    val cmdConfigs = profiles("cmdConfigs").asInstanceOf[Seq[String]]
    val bundles = profiles("bundles").asInstanceOf[List[Map[String, Any]]].map { bundleProfileMap =>
      BundleProfile(
        bundleProfileMap("className").asInstanceOf[String],
        bundleProfileMap("classArgs").asInstanceOf[Map[String, Any]],
        bundleProfileMap("delay").asInstanceOf[BigInt].toInt,
      )
    }
    DifftestProfile(cpu, numCores, cmdConfigs, bundles)
  }
}

object BundleProfile {
  def fromBundle(bundle: DifftestBundle, delay: Int): BundleProfile = {
    BundleProfile(bundle.getClass.getName, bundle.classArgs, delay)
  }
}

object Profile {
  def generateJson(
    cpu: String,
    cmdConfigs: Seq[String],
    bundles: Seq[(DifftestBundle, Int)],
    profileName: String = "difftest",
  ): Unit = {
    val difftestProfile = DifftestProfile.fromBundles(cpu, cmdConfigs, bundles)
    FileControl.write(Seq(difftestProfile.toJsonString), s"${profileName}_profile.json")
  }

  // This function generates the json profile for current DiffTest interfaces.
  def generateJson(cpu: String): Unit = {
    val cmdConfigs = DifftestModule.get_command_configs()
    val bundles = DifftestModule.get_current_interfaces()
    generateJson(cpu, cmdConfigs, bundles)
  }
}
