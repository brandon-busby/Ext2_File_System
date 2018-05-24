dd if=/dev/zero of=mydisk bs=1024 count=1440
      mkfs -b 1024 mydisk 1440  # enter y to let mkfs to proceed 
      mount -o loop mydisk /mnt
      (cd /mnt; ls; rmdir lost+found; ls)
      umount /mnt
