The target system must have an up-to-date version of Debian Linux
We can install this set of code in a few simple steps.\


1. Clone repository
2. Install python3.9 and pip3
3. Use crontab -e command in bash add the following to the crontab file '@reboot ./path_to_source/startup/ipLink.sh'
4. Reboot the device
5. Navigate to the source code directory and run the following command
6. Run ‘pip install cantools’
7. Then run the script with ‘python3 __main__.py’
8. Alternatively, you can add the command ‘python3 __main__.py’ to crontab on reboot to start the script on boot
