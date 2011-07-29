class matahari {
  package { ["matahari-host", "matahari-net", "matahari-service",
             "matahari-sysconfig", "matahari-lib", "matahari-agent-lib",
             "matahari-broker", "matahari-devel", "matahari"]:
             ensure => latest, require => [Yumrepo["fedora-updates"], Yumrepo["fedora"]] }

  service { [matahari-host, matahari-net, matahari-sysconfig, matahari-service,
             matahari-broker]: ensure => stopped, enable => true }
}
