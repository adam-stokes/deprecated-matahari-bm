Puppet::Type.type(:service).provide :systemd,
  :parent => :init, :source => :init do

  desc "execute systemd service using systemctl"

  commands :service => "/bin/systemctl"
  defaultfor :operatingsystem => [:fedora]
  def self.defpath
    superclass.defpath
  end

  def disable
    [command(:service), "disable", @resource[:name]]
  end

  def enabled?
    system("/bin/systemctl", "is-enabled", @resource[:name])
    if [0].include?($CHILD_STATUS.exitstatus)
      return :true
    else
      return :false
    end
  end

  def enable
    output = service(:enable, @resource[:name])
  rescue Puppet::ExecutionFailure => detail
    raise Puppet::Error, "Could not enable #{self.name}: #{detail}"
  end

  def restartcmd
    [command(:service), "restart", @resource[:name]]
  end

  def startcmd
    [command(:service), "start", @resource[:name]]
  end

  def stopcmd
    [command(:service), "stop", @resource[:name]]
  end
end
