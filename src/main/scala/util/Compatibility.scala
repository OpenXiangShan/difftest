// This file addresses compatibility issues for Chisel.

package difftest {

  import firrtl.ir.{Expression, Info, Statement, Type}

  import scala.reflect.runtime.currentMirror

  object compatibility {
    private def construct(className: String, arguments: Seq[Any]): Any = {
      val classSymbol = currentMirror.staticClass(className)
      val classMirror = currentMirror.reflectClass(classSymbol)
      val constructorSymbol = classSymbol.primaryConstructor.asMethod
      val constructorMirror = classMirror.reflectConstructor(constructorSymbol)
      constructorMirror.apply(arguments: _*)
    }

    // Return firrtl.ir.DefRegisterWithReset. Require Chisel 6.0.0 or higher.
    def DefRegisterWithReset(
      info: Info,
      name: String,
      tpe: Type,
      clock: Expression,
      reset: Expression,
      init: Expression,
    ): Statement = {
      val arguments: Seq[Any] = Seq(info, name, tpe, clock, reset, init)
      construct("firrtl.ir.DefRegisterWithReset", arguments).asInstanceOf[Statement]
    }

    // Return firrtl.ir.DefRegister. Require Chisel 6.0.0 or higher.
    def DefRegister(
      info: Info,
      name: String,
      tpe: Type,
      clock: Expression,
    ): Statement = {
      val arguments: Seq[Any] = Seq(info, name, tpe, clock)
      construct("firrtl.ir.DefRegister", arguments).asInstanceOf[Statement]
    }
  }
}

package chisel3 {

  import chisel3.internal._

  object compatibility {
    // Return the internal implicit Clock
    def currentClock: Option[Clock] = Builder.currentClock

    // Return the internal implicit Reset
    def currentReset: Option[Reset] = Builder.currentReset
  }

}
