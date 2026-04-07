/***************************************************************************************
 * Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
 * Copyright (c) 2025 Institute of Computing Technology, Chinese Academy of Sciences
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

import difftest.DifftestBundle
import difftest.common.FileControl

import scala.collection.mutable.ListBuffer

object Query {
  private val tables = ListBuffer.empty[QueryTable]
  private var batchTable: Option[BatchQueryTable] = None
  def register(gen: DifftestBundle, locPrefix: String, dutZone: String) = {
    tables += new QueryTable(gen, locPrefix, dutZone)
  }
  def registerBatch(gens: Seq[DifftestBundle], locPrefix: String, dutZone: String) = {
    gens.foreach { gen => tables += new QueryTable(gen, locPrefix, dutZone) }
    batchTable = Some(new BatchQueryTable(gens))
  }
  def writeInvoke(gen: DifftestBundle): String = {
    tables.find(_.gen == gen).get.writeInvoke
  }
  def collect() = {
    val queryCpp = ListBuffer.empty[String]
    queryCpp +=
      s"""
         |#ifndef __DIFFTEST_QUERY_H__
         |#define __DIFFTEST_QUERY_H__
         |
         |#include <cstdint>
         |#include "difftest-state.h"
         |#include "query.h"
         |#ifdef CONFIG_DIFFTEST_DELTA
         |#include "difftest-delta.h"
         |#endif // CONFIG_DIFFTEST_DELTA
         |
         |#ifdef CONFIG_DIFFTEST_QUERY
         |
         |class QueryStats: public QueryStatsBase {
         |public:
         |  ${tables.map { t => s"Query* ${t.instName};" }.mkString("\n  ")}
         |  ${batchTable.map(_.members).getOrElse("")}
         |  QueryStats(char *path): QueryStatsBase(path) {
         |    ${tables.map(_.initInvoke).mkString("\n    ")}
         |    ${batchTable.map(_.initInvoke).getOrElse("")}
         |  }
         |  ${tables.map(_.initDecl).mkString("")}
         |  ${tables.map(_.writeDecl).mkString("")}
         |  ${batchTable.map(_.initDecl).getOrElse("")}
         |  ${batchTable.map(_.writeDecl).getOrElse("")}
         |};
         |#endif // CONFIG_DIFFTEST_QUERY
         |#endif // __DIFFTEST_QUERY_H__
         |""".stripMargin
    FileControl.write(queryCpp, "difftest-query.h")
  }
}

class QueryTable(val gen: DifftestBundle, locPrefix: String, dutZone: String) {
  val tableName: String = gen.desiredModuleName.replace("Difftest", "")
  // Args: (key, value)
  private val stepArgs: Seq[(String, String)] = Seq(("STEP", s"query_step + (query_zone != $dutZone)"))
  private val locArgs: Seq[(String, String)] = {
    val argList = ListBuffer.empty[(String, String)]
    argList += (("COREID", "coreid"))
    // resolve conflict with sql key
    if (gen.isIndexed) argList += (("MY_INDEX", "index"))
    if (gen.isFlatten) argList += (("ADDRESS", "address"))
    argList.toSeq
  }
  private val dataArgs: Seq[(String, String)] = {
    if (gen.isDeltaElem) {
      Seq(("DATA", "*packet"))
    } else {
      val dataPrefix = "packet->"
      val argList = ListBuffer.empty[(String, String)]
      for ((name, _, elem) <- gen.dataElements) {
        val isRemoved = gen.isFlatten && Seq("valid", "address").contains(name)
        if (!isRemoved) {
          if (elem.length == 1) argList += ((name.toUpperCase, dataPrefix + name))
          else
            Seq.tabulate(elem.length) { idx =>
              argList += (((name + s"_$idx").toUpperCase, dataPrefix + name + s"[$idx]"))
            }
        }
      }
      argList.toSeq
    }
  }
  private val sqlArgs: Seq[(String, String)] = stepArgs ++ locArgs ++ dataArgs

  val instName: String = "query_" + tableName
  val initDecl =
    s"""
       |  void ${tableName}_init() {
       |    const char* createSql = \" CREATE TABLE $tableName(\" \\
       |      "ID INTEGER PRIMARY KEY AUTOINCREMENT," \\
       |      ${sqlArgs.map("\"" + _._1 + " INT NOT NULL").mkString("", ",\" \\\n      ", ");\";")}
       |    const char* insertSql = \"INSERT INTO $tableName (${sqlArgs.map(_._1).mkString(",")}) \" \\
       |      \" VALUES (${Seq.fill(sqlArgs.length) { "?" }.mkString(",")});\";
       |    $instName = new Query(mem_db, createSql, insertSql);
       |  }
       |""".stripMargin
  val initInvoke = s"${tableName}_init();"
  val packetType = if (gen.isDeltaElem) s"uint${gen.deltaElemWidth}_t" else gen.desiredModuleName
  val writeDecl = {
    val locParam = locArgs.map("uint8_t " + _._2).mkString(", ")
    s"""
       |  void ${tableName}_write(int dut_zone, $locParam, ${packetType}* packet) {
       |    query_${tableName}->write(${sqlArgs.length}, ${sqlArgs.map(_._2).mkString(", ")});
       |  }
       |""".stripMargin
  }
  val writeInvoke = s"qStats->${tableName}_write($dutZone, ${locArgs.map(locPrefix + _._2).mkString(", ")}, packet);"
}

class BatchQueryTable(template: Seq[DifftestBundle]) {
  private val bundleNames: Seq[String] =
    template.map(_.desiredModuleName.replace("Difftest", "")) ++ Seq("BatchStep", "BatchFinish")

  val members: String =
    s"""Query* query_BatchInfo;
       |  Query* query_BatchStep;""".stripMargin

  val initInvoke: String = "BatchTable_init();"

  val initDecl: String = {
    val bundleNameInserts = bundleNames.zipWithIndex.map { case (name, id) =>
      s"INSERT INTO BundleNames VALUES($id, '$name');"
    }.mkString("")

    s"""
       |  void BatchTable_init() {
       |    const char* createBatchInfoSql = "CREATE TABLE BatchInfo("
       |      "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
       |      "STEP INTEGER NOT NULL,"
       |      "BUNDLE_ID INTEGER NOT NULL,"
       |      "NUM INTEGER NOT NULL);";
       |    const char* insertBatchInfoSql = "INSERT INTO BatchInfo (STEP,BUNDLE_ID,NUM)"
       |      " VALUES (?,?,?);";
       |    query_BatchInfo = new Query(mem_db, createBatchInfoSql, insertBatchInfoSql);
       |
       |    const char* createBundleNamesSql = "CREATE TABLE BundleNames("
       |      "BUNDLE_ID INTEGER PRIMARY KEY,"
       |      "NAME TEXT NOT NULL);";
       |    char* errMsg;
       |    sqlite3_exec(mem_db, createBundleNamesSql, 0, 0, &errMsg);
       |    sqlite3_exec(mem_db, "$bundleNameInserts", 0, 0, &errMsg);
       |
       |    const char* createBatchStepSql = "CREATE TABLE BatchStep("
       |      "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
       |      "STEP INTEGER NOT NULL,"
       |      "${bundleNames.map(n => s"$n INTEGER NOT NULL").mkString(",")});";
       |    const char* insertBatchStepSql = "INSERT INTO BatchStep (STEP,${bundleNames.mkString(",")}) "
       |      "VALUES (${Seq.fill(bundleNames.length + 1)("?").mkString(",")});";
       |    query_BatchStep = new Query(mem_db, createBatchStepSql, insertBatchStepSql);
       |  }
       |""".stripMargin
  }

  val writeDecl: String = {
    val numArgs = bundleNames.indices.map(i => s"nums[$i]").mkString(", ")
    val writeArgCount = bundleNames.length + 1 // STEP + bundleNames
    s"""
       |  void BatchInfo_write(int bundle_id, int num) {
       |    query_BatchInfo->write(3, (int)query_step, bundle_id, num);
       |  }
       |  void BatchStep_write(int* nums) {
       |    query_BatchStep->write($writeArgCount, (int)query_step, $numArgs);
       |  }
       |""".stripMargin
  }
}
