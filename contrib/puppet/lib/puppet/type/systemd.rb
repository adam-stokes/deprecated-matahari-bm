require 'puppet/type'

Puppet::Type.newtype :systemd do
  @doc = "Run service via systemd"

  newparam :service do
    desc "Name of service"
  end

  newparam :action do
    desc "Start/Stop/Restart service"
  end

  ensurable do
    defaultvalues
    defaultto :start
  end
end
