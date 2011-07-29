$server = "puppet.example.com"

define configfile(owner = root, group = root, mode = 644, source,
                  backup = false, recurse = false, ensure = file) {
    file { $name:
            mode => $mode,
            owner => $owner,
            group => $group,
            backup => $backup,
            recurse => $recurse,
            ensure => $ensure,
            source => "puppet://$server/$source"
    }
}
