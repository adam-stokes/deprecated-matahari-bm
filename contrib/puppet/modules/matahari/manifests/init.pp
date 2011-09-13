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
              "matahari-host.service",
              "matahari-network.service",
              "matahari-sysconfig.service",
              "matahari-service.service",
              "matahari-broker.service"]
  service {
    $serlist:
      ensure => stopped,
      enable => true
  }
}
