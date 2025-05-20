## MISLOSCHI ALEXANDRA CORINA 324CA

# TEMA 3 - PCOM – Command-Line Client for REST API

## Overview

This C project implements a **command-line client (CLI)** that interacts with a RESTful server using HTTP. The application allows users to authenticate as **admin** or **user**, and perform operations related to user management, movies, and collections of movies.

The client communicates with a real backend server:
- **HOST:** `63.32.125.183`
- **PORT:** `8081`

## Features

The application supports the following features:

- Admin and user login/logout
- JWT-based access to a movie library
- Full management of users (admin only): add, list, delete
- Full management of movies: add, view, update, delete
- Management of collections: create, update, delete, add/remove movies

---

## Supported Commands (CLI)

### Admin Commands

| Command         | Description                       |
|----------------|-----------------------------------|
| `login_admin`   | Log in as admin                   |
| `logout_admin`  | Log out admin                     |
| `add_user`      | Add a new user                    |
| `get_users`     | View all registered users         |
| `delete_user`   | Delete a specific user            |

### User Commands

| Command        | Description                                |
|----------------|--------------------------------------------|
| `login`        | Log in as user with admin reference        |
| `logout`       | Log out user                               |
| `get_access`   | Get JWT token for accessing the library    |

### Movie Commands

| Command         | Description                         |
|-----------------|-------------------------------------|
| `get_movies`     | List all available movies          |
| `get_movie`      | View details of a specific movie   |
| `add_movie`      | Add a new movie                    |
| `delete_movie`   | Remove a movie by ID               |
| `update_movie`   | Update movie details               |

### Collection Commands

| Command                        | Description                                |
|--------------------------------|--------------------------------------------|
| `get_collections`              | List all collections                       |
| `get_collection`               | View details of a specific collection      |
| `add_collection`               | Create a new collection and add movies     |
| `delete_collection`            | Delete a collection by ID                  |
| `add_movie_to_collection`      | Add a movie to a specific collection       |
| `delete_movie_from_collection` | Remove a movie from a collection           |

---

## File Structure

### Command Handlers

- `commands/commands.c/h`: dispatch logic for commands
- `commands/admin_commands.c/h`: admin functionality (user management)
- `commands/user_commands.c/h`: user login and session handling
- `commands/movie_commands.c/h`: movie CRUD operations
- `commands/collection_commands.c/h`: collection CRUD operations

### Core & Helpers

- `client.c`: CLI main loop, calls `process_command`
- `helpers.c/h`: utility functions for input/response handling
- `http_requests.c/h`: builds HTTP requests (GET, POST, PUT, DELETE)
- `state.c/h`: manages session state (cookies, tokens, flags)

### JSON Support

- `parson.c/h`: Lightweight and easy-to-use JSON parser for C, used to decode all API responses and extract structured data such as tokens, error messages, and resource details (e.g., movies, collections).
- Parson was chosen for its simplicity, minimal overhead, and portability. It consists of a single `.c/.h` file, has a permissive license, and integrates easily into C projects without requiring external dependencies.
- It supports both JSON objects and arrays and allows safe, type-checked access to fields.
- The library is included locally in the project.
- 📦 **Source:** [Parson GitHub](https://github.com/kgabis/parson)

---

## Build & Run

### Compile

```bash
make
```

### Run

```bash
./client
```

### Clean

```bash
make clean
```

---

## Example Run

```
> login_admin
username=admin
password=admin123
SUCCESS: Admin logged in.

> add_user
username=test_user
password=pass123
SUCCESS: User created.

> logout_admin
SUCCESS: Admin logged out.

> login
admin_username=admin
username=test_user
password=pass123
SUCCESS: User logged in successfully.

> get_access
SUCCESS: JWT token received.

> add_movie
title=Interstellar
year=2014
description=A space exploration story.
rating=8.6
SUCCESS: Movie added.

> get_movies
SUCCESS: Lista filmelor
#1 Interstellar

> add_collection
title=SciFi Favorites
num_movies=1
movie_id[0]=1
SUCCESS: Collection "SciFi Favorites" created with id=5
SUCCESS: All movies added in the new collection

> get_collection
id=5
SUCCESS: Collection details
title: SciFi Favorites
owner: test_user
#1: Interstellar

> logout
SUCCESS: User logged out successfully.
```

---

## Summary

This CLI client is built with clear modularity, separating functionalities into specific files for better maintainability. JSON responses are parsed using the lightweight and efficient Parson library. Commands are intuitive and the client is easy to use and extend.