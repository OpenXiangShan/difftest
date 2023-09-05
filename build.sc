import mill._, scalalib._
import coursier.maven.MavenRepository

object ivys {
  val sv = "2.12.13"
  val chisel3 = ivy"edu.berkeley.cs::chisel3:3.5.0"
  val chisel3Plugin = ivy"edu.berkeley.cs:::chisel3-plugin:3.5.0"
}

trait CommonDiffTest extends ScalaModule with SbtModule {
  override def scalaVersion = ivys.sv

  override def scalacOptions = Seq("-Xsource:2.11")

  override def ivyDeps = Agg(ivys.chisel3)

  override def scalacPluginIvyDeps = Agg(ivys.chisel3Plugin)
}

object difftest extends SbtModule with CommonDiffTest {
  override def millSourcePath = os.pwd
}
