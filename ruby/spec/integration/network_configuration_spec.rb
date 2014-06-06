require 'spec_helper'

module Smith::Config
  describe 'Network configuration', :tmp_dir do
    extend WpaRoamFileHelper

    wpa_roam_file_setup
    
    def config_file(file_name)
      File.join(Smith.root, 'spec/resource', file_name)
    end

    scenario 'configure unsecured wireless network from file' do
      expect(Wireless).to receive(:enable_managed_mode)

      CLI.start(['load', config_file('unsecured.yml')])

      expect(wpa_roam_file).to contain_ssid('open_network')
      expect(wpa_roam_file).to contain_no_security
    end

    scenario 'configure wpa personal wireless network from file' do
      expect(Wireless).to receive(:enable_managed_mode)

      CLI.start(['load', config_file('wpa_personal.yml')])

      expect(wpa_roam_file).to contain_ssid('wpa_personal_network')
      expect(wpa_roam_file).to contain_psk('personal_passphrase')
    end

    scenario 'configure wpa enterprise wireless network from file' do
      expect(Wireless).to receive(:enable_managed_mode)

      CLI.start(['load', config_file('wpa_enterprise.yml')])

      expect(wpa_roam_file).to contain_ssid('wpa_enterprise_network')
      expect(wpa_roam_file).to contain_eap_credentials('enterprise_user', 'enterprise_pass', 'enterprise_domain')
    end

    scenario 'configure unsecured wireless network from hash' do
      expect(Wireless).to receive(:enable_managed_mode)

      Network.configure(security: 'none', ssid: 'open_network')

      expect(wpa_roam_file).to contain_ssid('open_network')
      expect(wpa_roam_file).to contain_no_security
    end

    scenario 'configure wireless network when wired interface is connected' do
      allow(Wired).to receive(:connected?).and_return(true)

      expect(Wireless).to receive(:enable_managed_mode)
      expect(Wireless).to receive(:disconnect)
      Network.configure(security: 'none', ssid: 'open_network')
    end

    scenario 'attempt configuration with non-existent file' do
      expect { CLI.start(['load', 'foo']) }.to raise_error(Errno::ENOENT)
      
    end

    scenario 'attempt configuration with invalid security type' do
      expect { Network.configure(security: 'other') }.to raise_error(InvalidNetworkConfiguration)
    end

    scenario 'attempt configuration with missing option' do
      expect { Network.configure(security: 'none') }.to raise_error(InvalidNetworkConfiguration)
    end

  end
end