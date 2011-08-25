class matahari::base {
  notice("Matahari manifests begin")

  $packagelist = [
                  "matahari-host",
                  "matahari-network",
                  "matahari-service",
                  "matahari-sysconfig",
                  "matahari-lib",
                  "matahari-agent-lib",
                  "matahari-broker",
                  "matahari"]
  package {
    $packagelist:  ensure => latest
  }

  $serlist = [
              "matahari-host",
              "matahari-network",
              "matahari-sysconfig",
              "matahari-service",
              "matahari-broker"]
  service {
    $serlist:
      ensure => stopped,
      enable => true
  }
}
