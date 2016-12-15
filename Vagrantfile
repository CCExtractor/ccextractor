Vagrant.configure(2) do |config|
  config.vm.box = "ubuntu/xenial64"

  # Uncomment this line if you want to sync other folders
  # config.vm.synced_folder "/home/user/video", "/video"

  config.vm.provision "shell", inline: <<-SHELL
    sudo apt-get install -y gcc
    sudo apt-get install -y libcurl4-gnutls-dev
    sudo apt-get install -y tesseract-ocr
    sudo apt-get install -y tesseract-ocr-dev
    sudo apt-get install -y libleptonica-dev
  SHELL

end

