
# Peer to Peer File Sharing System

A mini torrent-like peer-to-peer distributed file-sharing system with a single tracker, designed to handle user authentication, group management, and file sharing among peers.

## Prerequisites

- GCC Compiler for compiling C/C++ code
- Socket programming libraries
- Network setup to allow peer-to-peer communication

## Compile the Code

1. Compile the tracker and client:
   ```bash
   gcc -o tracker tracker.cpp
   gcc -o client client.cpp
   ```

## Running the Tracker

1. To start the tracker, run:
   ```bash
   ./tracker tracker_details.txt
   ```
   Ensure the `tracker_details.txt` file contains the necessary configuration information for the tracker to operate.

2. Note: Only one tracker is currently implemented, and it is required for managing connections between peers.

## Running the Client

1. Use the following command to start the client:
   ```bash
   ./client <IP>:<PORT> tracker_info.txt
   ```
   Replace `<IP>` and `<PORT>` with the appropriate IP address and port number of the tracker. The `tracker_info.txt` should have the necessary details to connect to the tracker.

## Supported Commands

The following commands are available for user interaction:

### User Management
- **Create User Account**: 
  ```bash
  create_user <user_id> <passwd>
  ```
- **Login**:
  ```bash
  login <user_id> <passwd>
  ```
- **Logout**:
  ```bash
  logout
  ```

### Group Management
- **Create Group**:
  ```bash
  create_group <group_id>
  ```
- **Join Group**:
  ```bash
  join_group <group_id>
  ```
- **Leave Group**:
  ```bash
  leave_group <group_id>
  ```
- **List Pending Join Requests**:
  ```bash
  list_requests <group_id>
  ```
- **Accept Group Joining Request**:
  ```bash
  accept_request <group_id> <user_id>
  ```
- **List All Groups In Network**:
  ```bash
  list_groups
  ```

## Usage Guidelines

1. **Creating an Account**: Before accessing the file-sharing network, users must create an account using the `create_user` command.
2. **Login**: Users must log in using their credentials to perform any other operations. Make sure to use:
   ```bash
   login <user_id> <passwd>
   ```
3. **Group Commands**: After logging in, users can create and join groups. Group administrators can also manage join requests.

### Example Workflow

1. **Create a user account**:
   ```bash
   create_user alice password123
   ```
2. **Login as Alice**:
   ```bash
   login alice password123
   ```
3. **Create a new group**:
   ```bash
   create_group tech-group
   ```
4. **Join an existing group**:
   ```bash
   join_group tech-group
   ```
5. **View pending join requests (as a group admin)**:
   ```bash
   list_requests tech-group
   ```
6. **Logout from the system**:
   ```bash
   logout
   ```

## Additional Notes

- Ensure that the tracker is running before attempting to start any client.
- Users need to log in before executing any commands other than `create_user`.
- Use secure passwords and avoid sharing them with others.