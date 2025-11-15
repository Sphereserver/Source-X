include(FetchContent)

# ------------------------------------------------------------------
# fetch_github_library():
#  - Derives the GitHub API URL for “latest release”
#  - Caches the JSON metadata locally
#  - Extracts `tag_name` from JSON (cached or freshly downloaded)
#  - Invokes FetchContent to clone/update into a local cache dir
#  - On offline/download failure, automatically reuses the existing cache
# ------------------------------------------------------------------

function(fetch_github_library project_name repo_url local_cache_dir)
    # project_name      e.g. "doctest"
    # repo_url          e.g. "https://github.com/doctest/doctest.git"
    # local_cache_dir   e.g. "${CMAKE_SOURCE_DIR}/external"

    # --------------------------------------------------------------
    # 1) Parse <owner> and <repo> from the provided Git URL
    # --------------------------------------------------------------
    string(REGEX MATCH
        "github\\.com[/:]([^/]+)/([^/]+)\\.git$"
        _ "${repo_url}"
    )
    #[[
    Parsing the GitHub owner/repo from a `.git` URL
    - `github\.com`
        - `\.`     → a literal dot. (In regex, `.` normally means “any character,” so we escape it to match an actual `.`.)
    - `[/:]`
        - A character class matching exactly one of the characters inside: either a slash `/` or a colon `:`.
        - This covers both URL styles:
            - HTTPS: `https://github.com/owner/repo.git`
            - SSH:   `git@github.com:owner/repo.git`
    - `([^/]+)`
        - `(` `)`  → defines a capturing group, so whatever matches inside will be saved as CMAKE_MATCH_1.
        - `[^/]`   → another character class, but with a `^` at the start: “anything but a slash.”
        - `+`      → “one or more” of the preceding item.
        - Together, `([^/]+)` means “capture one or more characters that are not `/`.” That gives us the owner.
    - `/`
        - Literal, not a special character. Separates owner from repo name.
    - `([^/]+)`
        - Same as the previous group, capturing the repo name.
    - `\.git`
        - Literal. Matches the suffix `.git`.
    - `$`
        - The “end-of-string” anchor. Ensures there’s nothing after `.git`.
    ]]

    if(NOT CMAKE_MATCH_COUNT EQUAL 2)
        message(FATAL_ERROR
            "Could not parse owner/repo from '${repo_url}'")
    endif()
    set(owner "${CMAKE_MATCH_1}")
    set(repo  "${CMAKE_MATCH_2}")

    # --------------------------------------------------------------
    # 2) Build the GitHub API URL for “latest release”
    # --------------------------------------------------------------
    set(api_url
        "https://api.github.com/repos/${owner}/${repo}/releases/latest"
    )

    # --------------------------------------------------------------
    # 3) Prepare cache paths (JSON + source tree)
    # --------------------------------------------------------------
    file(MAKE_DIRECTORY "${local_cache_dir}")
    if(NOT EXISTS "${local_cache_dir}")
        message(FATAL_ERROR
            "Failed to create cache directory: ${local_cache_dir}")
    endif()

    set(json_file        "${local_cache_dir}/${project_name}_latest.json")
    set(source_cache_dir "${local_cache_dir}/${project_name}")

    # --------------------------------------------------------------
    # 4) Try to read a cached JSON → tag_name
    # --------------------------------------------------------------
    set(lib_tag "")
    set(offline FALSE)

    if(EXISTS "${json_file}")
        message(STATUS "Found cached JSON for ${project_name}, validating…")
        file(READ "${json_file}" _data)
        string(REGEX MATCH "\"tag_name\"[ \t]*:[ \t]*\"([^\"]+)\"" _ "${_data}")
        #[[
        Extracting the `"tag_name"` value from the JSON
        - `"tag_name"`
            - A literal double-quoted string, matching exactly `"tag_name"`.
        - `[ \t]*`
            - A character class containing a space and `\t` (tab).
            - `*` means “zero or more” of that class.
            - So this matches any amount of whitespace (spaces or tabs) after `"tag_name"`.
        - `:`
            - A literal colon, since in JSON the key/value separator is `:`.
        - `[ \t]*`
            - Again, “any amount of spaces/tabs” after the colon.
        - `"`
            - A literal opening quote for the tag value.
        - `([^"]+)`
            - Capturing group #1.
            - `[^"]` is “any character except a quote.”
            - `+` is “one or more.”
            - So this grabs all characters inside the quotes (e.g. `v2.4.1`) until it hits the next `"`.
        - `"`
            - The closing literal quote.
        ]]
        if(CMAKE_MATCH_COUNT EQUAL 1)
            set(lib_tag "${CMAKE_MATCH_1}")
            message(STATUS "✔ Using cached tag: ${lib_tag}")
        else()
            message(WARNING "Cached JSON is invalid. Will attempt fresh download.")
        endif()
    endif()

    # --------------------------------------------------------------
    # 5) If no valid cached tag, download the latest JSON
    # --------------------------------------------------------------
    if(NOT lib_tag)
        message(STATUS "Downloading latest release info: ${api_url}")
        file(DOWNLOAD
            "${api_url}"          # ← remote
            "${json_file}"        # ← local destination
            STATUS dl_status
            TIMEOUT 5
            SHOW_PROGRESS
            INACTIVITY_TIMEOUT 5
        )
        list(GET dl_status 0 code)
        if(code EQUAL 0 AND EXISTS "${json_file}")
            file(READ "${json_file}" _data)
            string(REGEX MATCH "\"tag_name\"[ \t]*:[ \t]*\"([^\"]+)\"" _ "${_data}")
            if(CMAKE_MATCH_COUNT EQUAL 1)
                set(lib_tag "${CMAKE_MATCH_1}")
                message(STATUS "✔ Fetched tag: ${lib_tag}")
            else()
                message(WARNING "Failed to parse tag_name from downloaded JSON.")
                set(offline TRUE)
            endif()
        else()
            message(WARNING "Download failed (code=${code}). Will fall back to cache if any.")
            set(offline TRUE)
        endif()
    endif()

    # --------------------------------------------------------------
    # 6) Final sanity check: ensure we have a lib_tag
    # --------------------------------------------------------------
    if(NOT lib_tag)
        message(FATAL_ERROR
            "Could not determine a valid tag for ${project_name}. Aborting.")
    endif()

    # --------------------------------------------------------------
    # 7) Declare FetchContent variables *before* we call it
    #
    #    NEW:  In offline mode we must prevent both “git pull” and the
    #          very first “git clone”.  The trick is to:
    #
    #          a) Point CMake at the already-downloaded directory via
    #             FETCHCONTENT_SOURCE_DIR_<NAME>
    #          b) Disable updates via FETCHCONTENT_UPDATES_DISCONNECTED_<NAME>
    #
    #          These must be set *before* FetchContent_MakeAvailable().
    # --------------------------------------------------------------
    string(TOUPPER "${project_name}" UC_NAME)               # doctest → DOCTEST

    if(offline AND EXISTS "${source_cache_dir}/CMakeLists.txt")
        # ----------------------------------------------------------
        # We *have* source on disk and cannot reach the network →
        # pretend the project is “already populated”.
        # ----------------------------------------------------------
        set(FETCHCONTENT_SOURCE_DIR_${UC_NAME} "${source_cache_dir}")
        set(FETCHCONTENT_UPDATES_DISCONNECTED_${UC_NAME} ON)
        message(STATUS "Offline mode → reusing existing source at ${source_cache_dir}")
    else()
        message(STATUS "Online mode → CMake may clone or update ${project_name}")
    endif()

    # --------------------------------------------------------------
    # 8) Formal declaration (still points at the cache dir so both
    #    online and offline modes end up in the same place).
    # --------------------------------------------------------------
    FetchContent_Declare(
        ${project_name}
        GIT_REPOSITORY ${repo_url}
        GIT_TAG        ${lib_tag}
        SOURCE_DIR     "${source_cache_dir}"
    )

    # --------------------------------------------------------------
    # 9) Bring the dependency into the build
    # --------------------------------------------------------------
    FetchContent_MakeAvailable(${project_name})

endfunction()

