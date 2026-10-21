"""Small, bounded interpreter for the Lab3 three-address-code regression tests."""


def int32(value):
    return (value + 2**31) % 2**32 - 2**31


def divide(left, right):
    result = abs(left) // abs(right)
    return -result if (left < 0) != (right < 0) else result


def execute(text, inputs=(), step_limit=2_000_000):
    code = [line.split() for line in text.splitlines() if line.strip()]
    functions = {line[1]: pc + 1 for pc, line in enumerate(code) if line[0] == "FUNCTION"}
    labels = {line[1]: pc + 1 for pc, line in enumerate(code) if line[0] == "LABEL"}
    memory = {}
    next_address = 4096
    input_values = iter(inputs)
    output = []
    steps = 0

    def run(function, args):
        nonlocal next_address, steps
        variables = {}
        addresses = {}
        pending = []
        params = iter(args)

        def address(name, size=4):
            nonlocal next_address
            if name not in addresses:
                addresses[name] = next_address
                next_address += max(size, 4)
            return addresses[name]

        def value(operand):
            if operand.startswith("#"):
                return int(operand[1:])
            if operand.startswith("&"):
                return address(operand[1:])
            if operand.startswith("*"):
                return memory.get(value(operand[1:]), 0)
            if operand in addresses and addresses[operand] in memory:
                return memory[addresses[operand]]
            return variables[operand]

        def assign(name, result):
            result = int32(result)
            if name.startswith("*"):
                memory[value(name[1:])] = result
            elif name in addresses:
                memory[addresses[name]] = result
            else:
                variables[name] = result

        pc = functions[function]
        while pc < len(code):
            steps += 1
            if steps > step_limit:
                raise RuntimeError("IR step limit exceeded")
            line = code[pc]
            pc += 1
            op = line[0]
            if op == "LABEL":
                continue
            if op == "FUNCTION":
                raise RuntimeError("Function reached its end without RETURN")
            if op == "DEC":
                address(line[1], int(line[2]))
            elif op == "PARAM":
                assign(line[1], next(params))
            elif op == "ARG":
                pending.append(value(line[1]))
            elif op == "READ":
                assign(line[1], next(input_values))
            elif op == "WRITE":
                output.append(value(line[1]))
            elif op == "RETURN":
                return value(line[1])
            elif op == "GOTO":
                pc = labels[line[1]]
            elif op == "IF":
                left, right = value(line[1]), value(line[3])
                condition = {"==": left == right, "!=": left != right,
                             "<": left < right, ">": left > right,
                             "<=": left <= right, ">=": left >= right}[line[2]]
                if condition:
                    pc = labels[line[5]]
            elif len(line) >= 3 and line[1] == ":=":
                if line[2] == "CALL":
                    result = run(line[3], list(reversed(pending)))
                    pending.clear()
                elif len(line) == 3:
                    result = value(line[2])
                else:
                    left, right = value(line[2]), value(line[4])
                    operator = line[3]
                    if operator == "+":
                        result = left + right
                    elif operator == "-":
                        result = left - right
                    elif operator == "*":
                        result = left * right
                    elif operator == "/":
                        result = divide(left, right)
                    else:
                        raise ValueError(f"Unknown operator: {operator}")
                assign(line[0], result)
            else:
                raise ValueError(f"Unknown IR: {' '.join(line)}")
        raise RuntimeError(f"Missing RETURN in {function}")

    run("main", [])
    return output
