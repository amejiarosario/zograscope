#ifndef INTEGRATION_HPP__
#define INTEGRATION_HPP__

#include <memory>
#include <utility>

/**
 * @file integration.hpp
 *
 * @brief Several terminal integration utilities.
 */

/**
 * @brief A class that automatically spawns pager if output is large.
 *
 * Output must come to @c std::cout and is considered to be large when it
 * doesn't fit screen height.
 */
class RedirectToPager
{
public:
    class Impl;

    /**
     * @brief Can redirect @c std::cout until destruction.
     */
    RedirectToPager();

    //! No copy-constructor.
    RedirectToPager(const RedirectToPager &rhs) = delete;
    //! No copy-assignment.
    RedirectToPager & operator=(const RedirectToPager &rhs) = delete;

    /**
     * @brief Restores previous state of @c std::cout.
     */
    ~RedirectToPager();

public:
    /**
     * @brief Restores previous state of @c std::cout discharging destructor.
     */
    void discharge();

private:
    //! Implementation details.
    std::unique_ptr<Impl> impl;
};

/**
 * @brief Queries whether program output is connected to terminal.
 *
 * @returns @c true if so, otherwise @c false.
 */
bool isOutputToTerminal();

/**
 * @brief Retrieves terminal width and height in characters.
 *
 * @returns Pair of actual terminal width and height, or maximum possible values
 *          of the type.
 */
std::pair<unsigned int, unsigned int> getTerminalSize();

#endif // INTEGRATION_HPP__
