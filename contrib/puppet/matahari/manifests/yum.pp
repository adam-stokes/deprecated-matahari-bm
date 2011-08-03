yumrepo {
  "fedora" :
    mirrorlist => "https://mirrors.fedoraproject.org/metalink?repo=fedora-$releasever&arch=$basearch",
    enabled => 1, gpgcheck => 1,
    gpgkey => "file:///etc/pki/rpm-gpg/RPM-GPG-KEY-fedora-$basearch",
    require => File["/etc/pki/rpm-gpg/RPM-GPG-KEY-fedora-$basearch"];
  "fedora-updates" :
    mirrorlist => "https://mirrors.fedoraproject.org/metalink?repo=updates-released-f$releasever&arch=$basearch",
    enabled => 1, gpgcheck => 1,
    gpgkey => "file:///etc/pki/rpm-gpg/RPM-GPG-KEY-fedora-$basearch",
    require => File["/etc/pki/rpm-gpg/RPM-GPG-KEY-fedora-$basearch"]
}
