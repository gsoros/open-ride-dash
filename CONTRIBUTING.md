# Contributing to OpenRideDash

Thank you for your interest in contributing to OpenRideDash! This document provides guidelines and instructions for contributing to the project.

## Code of Conduct

Please read and follow our [Code of Conduct](CODE_OF_CONDUCT.md) to foster an open and welcoming environment.

## How Can I Contribute?

### Reporting Bugs

**Before submitting a bug report:**

1. Check if the bug has already been reported in [GitHub Issues](https://github.com/open-ride-dash/firmware/issues)
2. Test with the latest version from the `main` branch
3. Gather relevant information (logs, hardware configuration, steps to reproduce)

**Submitting a bug report:**

- Use the bug report template
- Include a clear title and description
- Provide steps to reproduce
- Include hardware/software configuration
- Add logs, screenshots, or videos if applicable

### Suggesting Enhancements

**Before suggesting an enhancement:**

1. Check if the feature has already been requested
2. Consider if the feature aligns with project goals
3. Think about implementation complexity and maintenance

**Submitting an enhancement suggestion:**

- Use the feature request template
- Explain the problem you're trying to solve
- Describe your proposed solution
- Discuss alternatives you've considered
- Include mockups or examples if applicable

### Contributing Code

#### Development Workflow

1. **Fork the repository**
2. **Create a feature branch**:
   ```bash
   git checkout -b feature/amazing-feature
   ```
3. **Make your changes**
4. **Write or update tests**
5. **Update documentation**
6. **Commit your changes**:
   ```bash
   git commit -m "Add amazing feature"
   ```
7. **Push to your fork**:
   ```bash
   git push origin feature/amazing-feature
   ```
8. **Open a Pull Request**

#### Pull Request Guidelines

- **Title**: Clear, descriptive title
- **Description**: Detailed explanation of changes
- **Related Issues**: Reference any related issues
- **Tests**: Confirm all tests pass
- **Documentation**: Update relevant documentation
- **Code Review**: Be responsive to feedback

### Contributing Documentation

Documentation improvements are highly valued! You can:

- Fix typos or clarify existing documentation
- Add missing documentation for features
- Create tutorials or how-to guides
- Improve code comments

### Testing

Help test OpenRideDash by:

- Testing on different hardware configurations
- Reporting edge cases
- Writing test cases
- Performing integration testing

### Community Support

Help others by:

- Answering questions on the forum
- Helping troubleshoot issues
- Reviewing pull requests
- Sharing your builds and experiences

## Development Setup

See [Development Setup Guide](docs/development/setup.md) for detailed instructions on setting up your development environment.

## Coding Standards

### C++ Code Style

- **Indentation**: 4 spaces, no tabs
- **Naming**:
  - Classes: `PascalCase`
  - Functions: `camelCase`
  - Variables: `camelCase`
  - Constants: `UPPER_SNAKE_CASE`
  - Private members: `m_camelCase` or `_camelCase`
- **Braces**: Allman style (braces on new line)
- **Line length**: 100 characters maximum
- **Headers**: Use `#pragma once`
- **Includes**: Order: system headers, library headers, project headers

### Example

```cpp
// Good
class CanManager {
public:
    void initialize(uint32_t baudrate) {
        if (baudrate == 0) {
            return;
        }

        m_baudrate = baudrate;
        setupHardware();
    }

private:
    uint32_t m_baudrate;
    void setupHardware();
};

// Avoid
class can_manager {
public:
void initialize(uint32_t baudrate) {
if(baudrate==0) return;
_baudrate=baudrate;
setup_hardware();}
private:
uint32_t _baudrate;
void setup_hardware();};
```

### Commit Messages

Use [Conventional Commits](https://www.conventionalcommits.org/) format:

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

**Types:**

- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, etc.)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Maintenance tasks

**Examples:**

```
feat(can): add support for MCP2515 CAN controller
fix(display): correct contrast calculation
docs(readme): update hardware requirements
```

### Documentation Standards

- Use Markdown for documentation
- Include code examples where helpful
- Keep documentation up-to-date with code changes
- Use relative links within the documentation

## Project Structure

```
open-ride-dash/
├── firmware/          # Main firmware repository
│   ├── src/          # Source code
│   ├── include/      # Header files
│   ├── test/         # Tests
│   └── platformio.ini
├── hardware/         # Hardware designs
├── mobile/           # Mobile application
├── docs/             # Documentation
└── tools/            # Development tools
```

## Testing Requirements

### Unit Tests

- Write unit tests for new functionality
- Maintain or improve test coverage
- Run tests before submitting PR:
  ```bash
  pio test -e native
  ```

### Hardware Testing

- Test on actual hardware when possible
- Document hardware configuration used for testing
- Consider edge cases (low voltage, high temperature, etc.)

### Integration Testing

- Test interactions between components
- Verify system behavior as a whole
- Test with different hardware configurations

## Review Process

### Pull Request Review

1. **Automated Checks**: CI runs tests and linting
2. **Maintainer Review**: At least one maintainer reviews
3. **Feedback**: Address any requested changes
4. **Merge**: Once approved, PR is merged

### Review Guidelines for Reviewers

- Be constructive and respectful
- Focus on code quality and maintainability
- Consider performance and security implications
- Check for documentation updates
- Verify tests are adequate

## Release Process

### Versioning

OpenRideDash uses [Semantic Versioning](https://semver.org/):

- **Major**: Breaking changes
- **Minor**: New features, backward compatible
- **Patch**: Bug fixes, minor improvements

### Release Cycle

1. **Feature freeze**: No new features, only bug fixes
2. **Testing phase**: Community testing on release candidates
3. **Release**: Tag version, create release notes
4. **Announcement**: Notify community via forum and social media

## Recognition

Contributors are recognized in:

- GitHub contributors list
- Release notes
- Project documentation (if significant contribution)
- Community announcements

## Getting Help

If you need help contributing:

1. **Check documentation** - This guide and other docs
2. **Search issues** - Similar questions may have been answered
3. **Ask on forum** - [forum.open-ride-dash.org](https://forum.open-ride-dash.org)
4. **Join Discord** - [discord.gg/open-ride-dash](https://discord.gg/open-ride-dash)
5. **Contact maintainers** - Via GitHub or email

## License

By contributing to OpenRideDash, you agree that your contributions will be licensed under the project's [MIT License](LICENSE).

---

Thank you for contributing to OpenRideDash! Your efforts help make e-bike technology more accessible and customizable for everyone.
