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

package difftest.common

import java.nio.charset.StandardCharsets
import java.nio.file.{Files, Paths, StandardOpenOption}

object FileControl {
  def write(fileStream: Iterable[String], fileName: String): Unit = write(fileStream, fileName, append = false)

  def write(fileStream: Iterable[String], fileName: String, append: Boolean): Unit = {
    val outputDir = Paths.get(sys.env("NOOP_HOME"), "build", "generated-src")
    write(fileStream, fileName, outputDir.toString, append)
  }

  def write(fileStream: Iterable[String], fileName: String, outputDir: String, append: Boolean): Unit = {
    Files.createDirectories(Paths.get(outputDir))
    val wmode = if (append) StandardOpenOption.APPEND else StandardOpenOption.TRUNCATE_EXISTING
    Files.write(
      Paths.get(outputDir, fileName),
      (fileStream.mkString("\n") + "\n").getBytes(StandardCharsets.UTF_8),
      Seq(StandardOpenOption.CREATE, wmode): _*
    )
  }
}
