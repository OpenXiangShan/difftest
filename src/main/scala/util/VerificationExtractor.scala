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

import firrtl._
import firrtl.annotations._
import firrtl.ir._
import firrtl.options.Phase
import firrtl.passes.wiring.{SinkAnnotation, SourceAnnotation}
import firrtl.stage.FirrtlCircuitAnnotation

// This is the main user interface for defining the Verification sink. Should be called only once.
// If any sink is defined, the following VerificationExtractor transform will perform the wiring.
object VerificationExtractor {
  def sink(cond: chisel3.Bool): Unit = {
    chisel3.experimental.annotate(new chisel3.experimental.ChiselAnnotation {
      override def toFirrtl: Annotation = VerificationExtractorSink(cond.toTarget)
    })
  }
}

// This transform converts firrtl.ir.Verification to Wiring when the sink is defined via annotation.
class VerificationExtractor extends Phase {
  // Legacy Chisel versions are not supported.
  require(!chisel3.BuildInfo.version.startsWith("3"), "This transform does not support Chisel 3.")

  implicit class AnnotationSeqHelper(annotations: AnnotationSeq) {
    def extractCircuit: (Circuit, Seq[Annotation]) = {
      val (circuitAnno, otherAnnos) = annotations.partition(_.isInstanceOf[FirrtlCircuitAnnotation])
      require(circuitAnno.length == 1, "no circuit?")
      (circuitAnno.head.asInstanceOf[FirrtlCircuitAnnotation].circuit, otherAnnos)
    }
  }

  override def invalidates(a: Phase) = false

  override def transform(annotations: AnnotationSeq): AnnotationSeq = {
    val (c, annos) = annotations.extractCircuit
    val (sinkAnnos, otherAnnos) = annos.partition(_.isInstanceOf[VerificationExtractorSink])
    // This transform runs only when the sink is defined
    if (sinkAnnos.nonEmpty) {
      require(sinkAnnos.length == 1, "cannot have more than one Verification sink")
      val circuitName = CircuitName(c.main)
      // Extract the Verification IRs and convert them into Sources
      val (sourceAnnos, modules) = c.modules.map(m => onSourceModule(m, circuitName)).unzip
      // Connect the Sources to the Sink module
      val sinkAnno = sinkAnnos.head.asInstanceOf[VerificationExtractorSink]
      val (sinkModules, otherModules) = modules.partition(_.name == sinkAnno.target.module)
      require(sinkModules.length == 1, "cannot have more than one Verification sink Module")
      require(sinkModules.head.isInstanceOf[Module], "Verification sink must be wrapper in some Module")
      val sinkModule = sinkModules.head.asInstanceOf[Module]
      val sinkTarget = sinkAnno.target.name
      val (newSinkAnnos, newSinkModule) = onSinkModule(sinkModule, circuitName, sinkTarget, sourceAnnos.flatten)
      // Concat all new modules and annotations
      val allModules = newSinkModule +: otherModules
      val allAnnos = otherAnnos ++ sourceAnnos.flatten ++ newSinkAnnos
      FirrtlCircuitAnnotation(c.copy(modules = allModules)) +: allAnnos
    } else {
      annotations
    }
  }

  private def onSourceModule(m: DefModule, c: CircuitName): (Seq[SourceAnnotation], DefModule) = {
    m match {
      case Module(info, name, ports, body) =>
        require(ports.exists(_.name == "reset"), "reset is required for converting assertions")
        val gen = new AssertionRegGenerator(ModuleName(name, c), Reference("reset", ResetType))
        val (regDefs, newBody) = onStmt(body)(gen)
        val sourceAnoos = gen.collect()
        (sourceAnoos, Module(info, name, ports, Block(regDefs :+ newBody)))
      case other: DefModule => (Seq(), other)
    }
  }

  private def onStmt(statement: Statement)(implicit regGen: AssertionRegGenerator): (Seq[Statement], Statement) = {
    statement match {
      case v: Verification =>
        val (reg, regRef) = regGen.next(v.clk)
        val conn = Connect(NoInfo, regRef, UIntLiteral(1))
        val cond = Conditionally(NoInfo, v.pred, EmptyStmt, conn)
        (Seq(reg), cond)
      case Conditionally(info, pred, conseq, alt) =>
        val (regs1, s1) = onStmt(conseq)
        val (regs2, s2) = onStmt(alt)
        (regs1 ++ regs2, Conditionally(info, pred, s1, s2))
      case Block(stmts) =>
        val (regs, s) = stmts.map(onStmt).unzip
        (regs.flatten, Block(s.filter(_ != EmptyStmt)))
      case x => (Seq(), x)
    }
  }

  private def onSinkModule(
    m: Module,
    circuitName: CircuitName,
    target: String,
    sources: Seq[SourceAnnotation],
  ): (Seq[SinkAnnotation], DefModule) = {
    require(m.ports.exists(_.name == "clock"), "clock is required for Verification sink Module")
    require(m.ports.exists(_.name == "reset"), "reset is required for Verification sink Module")
    val clock = Reference("clock", ClockType)
    val reset = Reference("reset", ResetType)
    val (sinkDefRegs, sinkDefRefs, sinkAnnos) = sources.map { case SourceAnnotation(_, pin) =>
      val (defReg, ref) = DefRegisterWithReset.withRef(NoInfo, pin, UIntType(IntWidth(1)), clock, reset, UIntLiteral(0))
      val conn = Connect(NoInfo, ref, UIntLiteral(0))
      val anno = SinkAnnotation(ComponentName(pin, ModuleName(m.name, circuitName)), pin)
      (Block(defReg, conn), ref, anno)
    }.unzip3
    val concat = sinkDefRefs.reduceLeft((result: Expression, sinkRef: Reference) =>
      DoPrim(PrimOps.Cat, Seq(sinkRef, result), Seq(), UIntType(IntWidth(1)))
    )
    val orReduce = DoPrim(PrimOps.Orr, Seq(concat), Seq(), UIntType(IntWidth(1)))
    val conn = Connect(NoInfo, Reference(target, UIntType(IntWidth(1))), orReduce)
    (sinkAnnos, m.copy(body = Block(m.body +: sinkDefRegs :+ conn)))
  }
}

private case class VerificationExtractorSink(target: ReferenceTarget) extends SingleTargetAnnotation[ReferenceTarget] {
  override def duplicate(n: ReferenceTarget): Annotation = this.copy(n)
}

private class AssertionRegGenerator(moduleName: ModuleName, reset: Expression) {
  val annos = scala.collection.mutable.ListBuffer.empty[SourceAnnotation]

  def next(clock: Expression): (Statement, Reference) = {
    val name = s"assertion_gen_${annos.length}"
    val (defReg, ref) = DefRegisterWithReset.withRef(NoInfo, name, UIntType(IntWidth(1)), clock, reset, UIntLiteral(0))
    annos.append(SourceAnnotation(ComponentName(name, moduleName), s"${moduleName.name}_$name"))
    (defReg, ref)
  }

  def collect(): Seq[SourceAnnotation] = annos.toSeq
}

// We define this DefRegisterWithReset to allow compiling the code in Chisel 3.x.
import scala.reflect.runtime.currentMirror

object DefRegisterWithReset {
  def withRef(
    info: Info,
    name: String,
    tpe: Type,
    clock: Expression,
    reset: Expression,
    init: Expression,
  ): (Statement, Reference) = {
    val classSymbol = currentMirror.staticClass("firrtl.ir.DefRegisterWithReset")
    val classMirror = currentMirror.reflectClass(classSymbol)
    val constructorSymbol = classSymbol.primaryConstructor.asMethod
    val constructorMirror = classMirror.reflectConstructor(constructorSymbol)
    val arguments: Seq[Any] = Seq(info, name, tpe, clock, reset, init)
    val defReg = constructorMirror.apply(arguments: _*).asInstanceOf[Statement]
    (defReg, Reference(name, tpe))
  }
}
