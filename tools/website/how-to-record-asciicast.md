# How to record an asciicasts for an example

1. `sudo pip3 install asciinema`
1. `asciinema auth` & click link
1. `echo "set -g default-shell /usr/bin/fish" >> ~/.tmux.conf`
1. Prepare tmux windows and commands
1. Ctrl-B+D to detach session
1. `asciinema rec -c "tmux attach"`
1. Call the executables in the prepared windows
1. Ctrl-D to stop after ~30 seconds
1. `sed 's/username/hypnotoad' /tmp/foo.cast`
1. `asciinema upload /tmp/foo.cast`
1. On [asciinema.org](https://asciinema.org): Make recording public & add link back to example
