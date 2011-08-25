class matahari::yum {
  package {
    yum: ensure => installed
  }

  file {
    fedora-updates:
      name => '/etc/yum.repos.d/fedora-updates.repo',
      ensure => present,
      owner => root,
      group => root,
      mode => 600,
      require => Package["yum"]
  }

  yumrepo {
    fedora-updates:
      descr => 'fedora-updates',
      baseurl => 'http://download.fedoraproject.org/pub/fedora/linux/updates/16/x86_64/',
      enabled => 1,
      gpgcheck => 0,
      require => File["fedora-updates"]
  }
}
